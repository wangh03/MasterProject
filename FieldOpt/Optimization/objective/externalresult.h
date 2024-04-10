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

#ifndef EXTERNALRESULT_H
#define EXTERNALRESULT_H

#include "objective.h"
#include "Settings/model.h"

namespace Optimization {
namespace Objective {

class ExternalResult : public Objective{
public:
/*!
 * \brief ExternalResult
 * \param settings The Settings object to get external result path
 * \param model The model object to do well cost calculation
 */
    ExternalResult(Settings::Optimizer *settings, Model::Model *model);
    double value() const override;
private:    
    class Component{
    public:
        std::string file_path;
        double passValueFromFile() const;
    };
    Component component;
    Settings::Optimizer *settings_;
    Model::Model::Economy *well_economy_;
};

} // Objective
} // Optimization

#endif //EXTERNALRESULT_H
