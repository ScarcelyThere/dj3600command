/* DeskJet3600.cpp: Implements the DeskJet3600, which reads a status from a
 *  Backend and parses it into Pens
 * Copyright (C) 2023 Scarcely There.
 *
 * commandtolidil is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any other version.
 *
 * commandtolidil is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the the GNU General Public License along with
 * commandtolidil. If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <cstring>
#include <string>
#include "DeskJet3600.hpp"
#include "USBBackend.hpp"

#ifdef TESTING
#  include "TestBackend.hpp"
#endif

#ifdef BUILD_HPMUD
#  include "HpmudBackend.h"
#endif

DeskJet3600::DeskJet3600 () :
    backend {NULL},
    numPens {0},
    curPen  {0}
{ }

DeskJet3600::DeskJet3600 (std::string uri) :
    deviceUri {uri},
    numPens   {0},
    curPen    {0}
{
    if (deviceUri.length () > 2 &&
        deviceUri.compare (0, 3, "usb") == 0)
    {
        backend = new USBBackend ();
        std::cerr << "DEBUG: USB backend selected" << std::endl;
    }
#ifdef BUILD_HPMUD
    else if (deviceUri.length () > 1 &&
        deviceUri.compare (0, 2, "hp") == 0)
    {
        backend = new HpmudBackend (uri.data ());
        std::cerr << "DEBUG: Selected HP backend" << std::endl;
    }
#endif
#ifdef TESTING
    else
    {
        backend = new TestBackend ();
        std::cerr << "DEBUG: Using test backend" << std::endl;
    }
#else
    else
        std::cerr << "ERROR: No compatible backend" << std::endl;
#endif
}

DeskJet3600::DeskJet3600 (DeskJet3600& source) :
    backend   {source.backend},
    deviceUri {source.deviceUri},
    numPens   {source.numPens},
    curPen    {source.curPen}
{
    for (int i = 0 ; i < source.numPens ; i++)
        pens[i] = new Pen(*source.pens[i]);
}

DeskJet3600::~DeskJet3600 ()
{
    if (backend)
        delete backend;

    clearPens ();
}

void
DeskJet3600::clearPens ()
{
    for (unsigned int i = 0 ; i < numPens ; i++ )
        delete pens[i];

    numPens = 0;
    curPen  = 0;
}

Pen*
DeskJet3600::firstPen ()
{
    curPen = 0;
return nextPen ();
}

bool
DeskJet3600::morePens ()
{
    return (curPen <= numPens);
}

Pen*
DeskJet3600::nextPen ()
{
    return pens[curPen++];
}

int
DeskJet3600::update ()
{
    if (backend == NULL)
        return 0;

    if (backend->getDeviceID (deviceID))
    {
        clearPens ();
        return parseStatus ();
    }
    else
        return 0;
}

int
DeskJet3600::parseStatus ()
{
    // Both offsets are in hexadecimal digits
    const int  penDataOffset = 18,
               penDataLength = 8;

    size_t offset;

    // Where's the relevant hex status string?
    offset = deviceID.find (";S:");
    if (offset == deviceID.npos)
        return 0;

    // The pens are found at string offset 18, after the ";S:"
    offset += 3;
    if (deviceID.length () < offset + penDataOffset)
        return 0;

    std::string relevantStatus = deviceID.substr (offset);
    char const *rawStatus = relevantStatus.c_str ();

    unsigned int revision;
    sscanf (rawStatus, "%2x", &revision);

    // DeskJet 3600 is revision 3, so anything else won't work here
    if (revision != 3)
        return 0;

    unsigned int penCount,
                 curRawPen;

    rawStatus += penDataOffset;

    // First nybble is the number of pens, which we obtain and then
    //  skip over
    sscanf (rawStatus++, "%1x", &penCount);

    // DeskJet 3600 has two cartridge slots. Any more and it's not that
    //  printer model.
    if (penCount > 2)
        return 0;

    Pen* pen;
    for (unsigned int i = 0 ; i < penCount ; i++)
    {
        sscanf (rawStatus, "%8x", &curRawPen);
        try
        {
            pen = new Pen (curRawPen);
            pens[numPens++] = pen;
        }
        catch (const InvalidPenException &e)
        {
            std::cout << "DEBUG: Invalid Pen discovered" << std::endl;
        }

        rawStatus += penDataLength;
    }

    return 1;
}

// The following LIDIL packets were lifted from HP's own ldl.py, by Don
//  Welch. Someday, I'd love to add some of this functionality to the
//  hp backend, but for now, I'm just implementing it here.

void
DeskJet3600::printAlignmentPage ()
{
    // Send the LIDIL packet to print the built-in page to the rest of CUPS
    //  via standard output
    
    // This is only valid for command packets, I think.
    // Packet format: F< size >< 0><cm><ac><      ><      >
    //  cm: Packet type (command)
    //  ac: The actual command byte
    // Header:        <          10 bytes                 >
    char command[] = "$\x00\x10\x00\x00\x0c\x00\x00\x00\x00\x11\xff\xff\xff\xff$";
    std::cout.write (command, sizeof (command));
    std::cout.flush ();
}

void
DeskJet3600::clean ()
{
    char command[] = "$\x00\x10\x00\x00\x08\x00\x00\x00\x00\x03\xff\xff\xff\xff$";
    std::cout.write (command, sizeof (command));
    std::cout.flush ();
}

// vim: et sw=4
