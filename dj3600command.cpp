/* dj3600command: an ink status utility for CUPS-powered DeskJet F4180 all-in-ones
 *  Copyright (C) 2023 Scarcely There.
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

#include <iostream>
#include <fstream>
#include <cstring>
#include <signal.h>
#include "Pen.h"
#include "DeskJet3600.h"

// Just a little forward declaration of a local function
void sendCupsLevels (DeskJet3600&);

int main (int argc, char* argv[])
{
    signal (SIGPIPE, SIG_IGN);

    if (argc != 7)
    {
        // No way we're being called from CUPS.
        std::cerr << "ERROR: insufficient or too many arguments" << std::endl;
        return 1;
    }

    char* deviceUri = getenv ("DEVICE_URI");
    if (!deviceUri)
    {
        std::cerr << "ERROR: no DEVICE_URI environment variable" << std::endl;
        return 1;
    }

    std::istream jobFile (argv[6], std::ios_base::in);
    if (!jobFile)
        return 1;

    DeskJet3600 printer (deviceUri);

    std::string jobLine;
    while (jobFile >> jobLine)
    {
        std::cerr << "DEBUG: jobLine is " << jobLine << std::endl;

        // "Clean all" instead of "Clean" as specified threw me off a little
        if (jobLine == "Clean")
            printer.clean ();
        else if ()
            printer.printAlignmentPage ();
        else if (0 == strcmp (jobLine, "ReportLevels"))
        {
            if (printer.updateStatus ())
                sendCupsLevels (printer);
            else
            {
                // This is probably bad enough that we just stop.
                std::cerr << "ERROR: Could not get supply levels" << std::endl;
                return 1;
            }
        }
        // Any other line is unsupported or a comment. Ignore it.
    }

    return 0;
}

void
sendCupsLevels (DeskJet3600& printer)
{
    // I thought about a CUPS object, but why?
    std::string markerTypes  = "ATTR:marker-types=";
    std::string markerLevels = "ATTR:marker-levels=";
    std::string markerColors = "ATTR:marker-colors=";
    std::string markerNames  = "ATTR:marker-names=";
    
    Pen* curPen;
    bool firstPass = true;
    for (curPen = printer.firstPen () ; curPen != NULL ;
            curPen = printer.nextPen ())
    {
        if (firstPass)
        {
            // We'll need commas after this
            firstPass = false;
        }
        else
        {
            // commas needed
            markerTypes  += ",";
            markerLevels += ",";
            markerColors += ",";
            markerNames  += ",";
        }

        markerTypes  += curPen->markerType ();
        markerLevels += std::to_string (curPen->getLevel ());
        markerColors += curPen->toHex ();
        markerNames  += curPen->name ();
    }

    std::cerr << markerTypes  << std::endl;
    std::cerr << markerLevels << std::endl;
    std::cerr << markerColors << std::endl;
    std::cerr << markerNames  << std::endl;
}

// vim: et sw=4
