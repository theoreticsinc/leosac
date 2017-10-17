/*
    Copyright (C) 2014-2017 Leosac

    This file is part of Leosac.

    Leosac is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Leosac is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "hardware/Device.hpp"
#include "hardware/HardwareFwd.hpp"
#include <cstdint>
#include <odb/callback.hxx>
#include <odb/core.hxx>
#include <string>

namespace Leosac
{
namespace Hardware
{

/**
 *  Abstraction of a GPIO device attributes.
 *
 *  Modules that provides GPIO support may subclass
 *  this to provide additional configuration options.
 */
#pragma db object callback(validation_callback) table("HARDWARE_GPIO")
class GPIO : public Device
{
  public:
    enum class Direction
    {
        In  = 0,
        Out = 1
    };

    explicit GPIO();
    virtual ~GPIO() = default;

    uint16_t number() const;

    void number(uint16_t number);

    Direction direction() const;

    void direction(Direction direction);

    bool default_value() const;

    void default_value(bool default_value);

    void validation_callback(odb::callback_event e, odb::database &) const override;

  private:
    uint16_t number_;

    Direction direction_;

    /**
     * True to default to ON, false otherwise.
     */
    bool default_value_;

    friend odb::access;
};
}
}
