/*******************************************************************************
 * Created: 2024 by Wang Hou
 *
 *  This file is part of the FieldOpt project.
 *
 *  Copyright (C) 2015-2024 Wang Hou <wang.hou@ntnu.no>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 ******************************************************************************/

#include "externalresult.h"
#include "Model/model.h"

namespace Optimization {
namespace Objective {

ExternalResult::ExternalResult(Settings::Optimizer *settings, Model::Model *model) {
    settings_ = settings;
    well_economy_ = model->wellCostConstructor();
    component.file_path = settings_->objective().external_result.external_file_path;
}

double ExternalResult::value() const {
    double value = 0;

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
    double external_objective_value = component.passValueFromFile();
    value += external_objective_value;

    return value;
}

double ExternalResult::Component::passValueFromFile() const {
    double result;
    std::ifstream resultfile(file_path);
    if (resultfile.is_open()) {
        resultfile >> result;
        resultfile.close();
    } else {
        std::cerr << "Unable to get the result form " << file_path << std::endl;
    }
    return result;
}

} // Objective
} // Optimization