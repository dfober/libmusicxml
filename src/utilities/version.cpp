/*
  MusicXML Library
  Copyright (C) Grame 2006-2013

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr
*/

#include <iostream>
#include <iomanip>      // setw, setprecision, ...

#include "version.h"

#include "utilities.h"


using namespace std;

namespace MusicXML2
{

//______________________________________________________________________________
list<versionInfo> gVersionInfoList;

versionInfo::versionInfo (
  string versionNumber,
  string versionDate,
  string versionDescription)
{
  fVersionNumber      = versionNumber;
  fVersionDate        = versionDate;
  fVersionDescription = versionDescription;
}

versionInfo::~versionInfo ()
{}

void versionInfo::print (ostream& os) const
{
  os << left <<
    outputIndenter::gGlobalOStreamIndenter <<
      fVersionNumber <<
      " (" << fVersionDate << "):" <<
      endl <<

  ++outputIndenter::gGlobalOStreamIndenter;

  os <<
    outputIndenter::gGlobalOStreamIndenter <<
      fVersionDescription;

  --outputIndenter::gGlobalOStreamIndenter;

  os << endl;
}

void appendVersionToVersionInfoList (
  string versionNumber,
  string versionDate,
  string versionDescription)
{
  gVersionInfoList.push_front (
    versionInfo (
      versionNumber,
      versionDate,
      versionDescription));
}

void printVersionsHistory (
  std::string   executable,
  std::ostream& os)
{
  os <<
    executable + " versions history:" <<
    endl;

  ++outputIndenter::gGlobalOStreamIndenter;

  list<versionInfo>::const_iterator
    iBegin = gVersionInfoList.begin (),
    iEnd   = gVersionInfoList.end (),
    i      = iBegin;
  for ( ; ; ) {
    (*i).print (os);
    if (++i == iEnd) break;
  // JMI  os << endl;
  } // for

  --outputIndenter::gGlobalOStreamIndenter;
}

string currentVersionNumber ()
{
  return
    gVersionInfoList.front ().fVersionNumber;
}


}
