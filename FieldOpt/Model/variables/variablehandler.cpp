/******************************************************************************
 *
 *
 *
 * Created: 20.10.2015 2015 by einar
 *
 * This file is part of the FieldOpt project.
 *
 * Copyright (C) 2015-2015 Einar J.M. Baumann <einar.baumann@ntnu.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *****************************************************************************/

#include "variablehandler.h"
#include "variable_exceptions.h"
#include <iostream>

namespace Model {
namespace Variables {

VariableHandler::VariableHandler(Utilities::Settings::Model settings)
{
    wells_ = QList<Well *>();
    for (int well_idx = 0; well_idx < settings.wells().size(); ++well_idx) { // Wells
        Utilities::Settings::Model::Well settings_well = settings.wells()[well_idx];
        Well *new_well = new Well(settings_well.name);
        for (int control_time = 0; control_time < settings.control_times().size(); ++control_time) {
            new_well->controls_.append(new Control (settings.control_times()[control_time]));
        }
        for (int completion_nr = 0; completion_nr < settings_well.completions.size(); ++completion_nr) {
            if (settings_well.completions[completion_nr].type == ::Utilities::Settings::Model::WellCompletionType::Perforation)
                new_well->perforations_.append(new Perforation(settings_well.completions[completion_nr]));
        }
        for (int var_idx = 0; var_idx < settings_well.variables.size(); ++var_idx) { // Variables
            Utilities::Settings::Model::Well::Variable settings_var = settings_well.variables[var_idx];
            switch (settings_var.type) {
            case Utilities::Settings::Model::WellVariableType::BHP :
                for (int var_time_idx = 0; var_time_idx < settings_var.time_steps.size(); ++var_time_idx) { // Variable time steps
                    int control_index = wellControlIndex(new_well, settings_var.time_steps[var_time_idx]);
                    new_well->controls_[control_index]->bhp_ = true;
                }
                break;
            case Utilities::Settings::Model::WellVariableType::Rate:
                for (int var_time_idx = 0; var_time_idx < settings_var.time_steps.size(); ++var_time_idx) { // Variable time steps
                    int control_index = wellControlIndex(new_well, settings_var.time_steps[var_time_idx]);
                    new_well->controls_[control_index]->rate_ = true;
                }
                break;
            case Utilities::Settings::Model::WellVariableType::OpenShut:
                for (int var_time_idx = 0; var_time_idx < settings_var.time_steps.size(); ++var_time_idx) { // Variable time steps
                    int control_index = wellControlIndex(new_well, settings_var.time_steps[var_time_idx]);
                    new_well->controls_[control_index]->open_ = true;
                }
                break;
            case Utilities::Settings::Model::WellVariableType::Transmissibility:
                for (int block_nr = 0; block_nr < settings_var.blocks.size(); ++block_nr) {
                    new_well->getPerforation(&settings_var.blocks[block_nr].position)->transmissibility_factor_ = true;
                }
                break;
            }
        }
        wells_.append(new_well);
    }
}

VariableHandler::Control *VariableHandler::GetControl(QString well_name, int time)
{
    for (int i = 0; i < wells_.size(); ++i) {
        if (QString::compare(well_name, wells_[i]->name()) == 0) {
            for (int j = 0; j < wells_[i]->controls_.size(); ++j) {
                if (wells_[i]->controls_[j]->time_ == time) {
                    return wells_[i]->controls_[j];
                }
            }
        }
    }
    throw VariableHandlerCannotFindObjectException("The variable handler was unable to find a control point for time " + std::to_string(time) + " in well " + well_name.toStdString());
}

VariableHandler::Perforation *VariableHandler::GetPerforation(int completion_id)
{
    for (int i = 0; i < wells_.size(); ++i) {
        for (int j = 0; j < wells_[i]->perforations_.size(); ++j) {
            if (wells_[i]->perforations_[j]->id_ == completion_id)
                return wells_[i]->perforations_[j];
        }
    }
    throw VariableHandlerCannotFindObjectException("The variable handler was unable to find a perforation with id " + std::to_string(completion_id));
}

VariableHandler::Well *VariableHandler::GetWell(QString well_name)
{
    for (int i = 0; i < wells_.size(); ++i) {
        if (QString::compare(well_name, wells_[i]->name()) == 0)
            return wells_[i];
    }
    throw VariableHandlerCannotFindObjectException("The variable handler was unable to find a well named " + well_name.toStdString());
}


int VariableHandler::wellControlIndex(VariableHandler::Well *well, int time)
{
    for (int i = 0; i < well->controls_.size(); ++i) {
        if (well->controls_[i]->time() == time) return i;
    }
    return -1;
}

VariableHandler::Perforation *VariableHandler::Well::getPerforation(int id)
{
    for (int i = 0; i < perforations_.size(); ++i) {
        if (id == perforations_[i]->id_)
            return perforations_[i];
    }
    throw VariableHandlerCannotFindObjectException("The variable handler was unable to find a perforation with id " + std::to_string(id));
}

VariableHandler::Perforation *VariableHandler::Well::getPerforation(Utilities::Settings::Model::IntegerCoordinate *block)
{
    for (int i = 0; i < perforations_.size(); ++i) {
        if (perforations_[i]->block_.position.Equals(block))
            return perforations_[i];
    }
    throw VariableHandlerCannotFindObjectException("The variable handler was unable to find a perforation at the given block.");
}

}
}

