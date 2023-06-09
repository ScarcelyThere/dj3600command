/* Pen.cpp: Implements the Pen class, which parses part of a HP printer's status and
 *  represents a cartridge, ink tank, or print head
 * Copyright (C) 2023 Scarcely There.
 *
 * dj3600command is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any other version.
 *
 * dj3600command is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the the GNU General Public License along with
 * dj3600command. If not, see <https://www.gnu.org/licenses/>.
 */

#include "Pen.h"

Pen::Pen () :
    color {Color::None},
    type  {Type::Invalid},
    level {0}
{ }

Pen::Pen (Pen& source) :
    color {source.getColor () },
    type  {source.getType ()  },
    level {source.getLevel () }
{ }

Pen& Pen::operator= (Pen& source)
{
    if (this == &source)
        return *this;

    color = source.getColor ();
    type  = source.getType ();
    level = source.getLevel ();

    return *this;
}

Pen::Pen (const unsigned int status) : Pen ()
{
    // Lowest byte is the level
    level = status & 0xFF;

    // The type ("kind" in HP parlance) is the top two bits
    unsigned int kind = (status & 0xC0000000) >> 30;
    if (kind >= Type::lastValidType)
    {
        type = Type::Unknown;
    }
    else
        type = TypeDescs[kind].type;

    // The color ("type" in HP parlance) is the next six bits
    unsigned int hpType = (status & 0x3F000000) >> 24;
    if (hpType >= Color::lastValidColor)
    {
        color = Color::Error;
    }
    else
        color = ColorDescs[hpType].color;
}

std::string
Pen::markerType ()
{
    if (type == Type::Invalid)
        return "invalid";
    else
        return TypeDescs[type].name;
}

std::string
Pen::toHex ()
{
    if (color == Color::Error)
        return "error";
    else
        return ColorDescs[color].rgbDesc;
}

std::string
Pen::name ()
{
    if (color == Color::Error)
        return "error";
    else
        return ColorDescs[color].enDesc;
}

Pen::Pen (const unsigned short status) : Pen ()
{
    // TODO: Implement this so we can support more printers
    (void)status;
};

int
Pen::getLevel ()
{
    return level;
}

Pen::Color
Pen::getColor ()
{
    return color;
}

bool
Pen::exists ()
{
    // So far, if there's any marking Pen with no color, it
    //  may not be installed and not actually exist in the
    //  device.
    bool marking = type == Type::Cartridge ||
                   type == Type::Tank ||
                   type == Type::Printhead;

    return (marking && color != Color::None
                    && color != Color::Error);
}

Pen::Type
Pen::getType (void)
{
    return type;
}

// vim: et sw=4
