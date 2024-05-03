/******************************************************************************
Copyright (C) 2015-2017 Brage S. Kristoffersen <brage_sk@hotmail.com>
Modified by Wang Hou (2015-2024) <wang.hou@ntnu.no>

This file is part of the FieldOpt project.

FieldOpt is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

FieldOpt is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with FieldOpt.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/
#include "NPV.h"

#include <iostream>
#include <iomanip>
#include "weightedsum.h"
#include <stdlib.h>
#include <cmath>
#include "Model/model.h"
#include "Model/wells/well.h"
#include <Utilities/printer.hpp>

using std::cout;
using std::endl;
using std::fixed;
using std::setprecision;
using std::scientific;

namespace Optimization {
namespace Objective {

NPV::NPV(Settings::Optimizer *settings,
         Simulation::Results::Results *results,
         Model::Model *model) {
  settings_ = settings;
  results_ = results;
  components_ = new QList<NPV::Component *>();

  for (int i = 0; i < settings->objective().NPV_sum.size(); ++i) {
    auto *comp = new NPV::Component();
    if (settings->objective().NPV_sum[i].property.compare(0, 4, "EXT-") == 0 ) {
        comp->is_json_component = true;
        Printer::ext_info("Adding external NPV component.", "Optimization", "NPV");
        comp->property_name = settings->objective().NPV_sum[i].property.substr(4, std::string::npos);
        comp->interval = settings->objective().NPV_sum[i].interval;
    }
    else {
        comp->is_json_component = false;
        comp->property_name = settings->objective().NPV_sum.at(i).property;
        comp->property = results_->GetPropertyKeyFromString(QString::fromStdString(comp->property_name));
    }
    comp->coefficient = settings->objective().NPV_sum.at(i).coefficient;
    if (settings->objective().NPV_sum.at(i).usediscountfactor == true) {
      comp->interval = settings->objective().NPV_sum.at(i).interval;
      comp->discount = settings->objective().NPV_sum.at(i).discount;
      comp->usediscountfactor = settings->objective().NPV_sum.at(i).usediscountfactor;
    } else {
      comp->interval = "None";
      comp->discount = 0;
      comp->usediscountfactor = false;
    }
    if (settings->objective().NPV_sum.at(i).is_well_prop) {
        comp->is_well_property = true;
        comp->well = settings->objective().NPV_sum.at(i).well;
      }
    else comp->is_well_property = false;
    components_->append(comp);
  }
  well_economy_ = model->wellCostConstructor();
}

double NPV::value() const {
  try {
  double value = 0;
  auto report_times = results_->GetValueVector(results_->Time);
  auto NPV_times = new QList<int>;
  auto NPV_report_times = new QList<int>;
  auto discount_factor_list = new QList<double>;
  for (int comp_index = 0; comp_index < components_->size(); ++comp_index) {
    if (components_->at(comp_index)->is_json_component == true) {
        continue;
    }
    if (components_->at(comp_index)->interval == "Yearly") {
      double discount_factor;
      for (int report_time_index = 0; report_time_index < report_times.size(); report_time_index++) {
        if (report_time_index < report_times.size() - 1 &&  (report_times[report_time_index+1] - report_times[report_time_index]) > 365) {
            std::stringstream ss;
            ss << "Skipping assumed pre-simulation time step " << report_times[report_time_index]
               << ". Next time step: " << report_times[report_time_index+1] << ". Ignore if this is time 0 in a restart case.";
            Printer::ext_warn(ss.str(), "Optimization", "NPV");
        }
        auto moded_yearly_report_time = (int) (report_times.at(report_time_index) - std::fmod(report_times.at(report_time_index), 365)) / 365;
        if (!NPV_times->contains(moded_yearly_report_time)) {
            NPV_times->append(moded_yearly_report_time);
          NPV_report_times->append(report_time_index);
          discount_factor = 1 / (1 * (pow(1 + components_->at(comp_index)->discount, moded_yearly_report_time)));
          discount_factor_list->append(discount_factor);
        }
      }
    }else if (components_->at(comp_index)->interval == "Monthly") {
        double discount_factor;
        double discount_rate = components_->at(comp_index)->discount;
        double monthly_discount = components_->at(comp_index)->yearlyToMonthly(discount_rate);
        for (int report_time_index = 0; report_time_index < report_times.size(); report_time_index++) {
            auto moded_monthly_report_time = (int) (report_times.at(report_time_index) - std::fmod(report_times.at(report_time_index), 30)) / 30;
            if (!NPV_times->contains(moded_monthly_report_time)) {
                NPV_times->append(moded_monthly_report_time);
                NPV_report_times->append(report_time_index);
            discount_factor = 1 / (1 * (pow(1 + monthly_discount, moded_monthly_report_time)));
            discount_factor_list->append(discount_factor);
          }
        }
      }
    }

    for (int comp_index = 0; comp_index < components_->size(); ++comp_index) {
      if (components_->at(comp_index)->is_json_component == true) {
          continue;
      }
      if (components_->at(comp_index)->usediscountfactor == true) {
        for (int NPV_report_index = 1, discount_index = 0; NPV_report_index < NPV_report_times->size(); ++NPV_report_index, ++discount_index) {
          auto prod_difference = components_->at(comp_index)->resolveValueDiscount(results_, NPV_report_times->at(NPV_report_index))
              - components_->at(comp_index)->resolveValueDiscount(results_, NPV_report_times->at(NPV_report_index - 1));
          value += prod_difference * components_->at(comp_index)->coefficient * discount_factor_list->at(discount_index);
        }
      } else if (components_->at(comp_index)->usediscountfactor == false) {
        value += components_->at(comp_index)->resolveValue(results_);
        std::string prop_name = components_->at(comp_index)->property_name;
      }
    }

    if (well_economy_->use_well_cost) {
      for (auto well: well_economy_->wells_pointer) {
        if (well_economy_->separate) {
          value -= well_economy_->costXY * well_economy_->well_xy[well->name().toStdString()];
          value -= well_economy_->costZ * well_economy_->well_z[well->name().toStdString()];
        } else {
          value -= well_economy_->cost * well_economy_->well_lengths[well->name().toStdString()];
        }
      }
    }
    for (int j = 0; j < components_->size(); ++j) {
      if (components_->at(j)->is_json_component == true) {
          if (components_->at(j)->interval == "Single" || components_->at(j)->interval == "None") {
              double extval = components_->at(j)->coefficient
                  * results_->GetJsonResults().GetSingleValue(components_->at(j)->property_name);
              value += extval;

          }
          else {
            Printer::ext_warn("Unable to parse external component.", "Optimization", "NPV");
          }
      }
    }
    return value;
  }
  catch (...) {
    Printer::error("Failed to compute NPV. Returning 0.0");
    return 0.0;
  }
}

double NPV::Component::resolveValue(Simulation::Results::Results *results) const {
  return coefficient * results->GetValue(property);

}
double NPV::Component::resolveValueDiscount(Simulation::Results::Results *results, int time_step) const {
    if (is_well_property){
        return results->GetValue(property, well, time_step);
    }
    else{
        return results->GetValue(property,time_step);
    }
}

double NPV::Component::yearlyToMonthly(double discount_factor) {
   // assume a month length of 30 days , 30/365 = 0.0821917
    return pow((1 + discount_factor), 0.0821917) - 1;

}

}

}
