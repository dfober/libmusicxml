/*
  MusicXML Library
  Copyright (C) Grame 2006-2013

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr
*/

#ifndef ___musicxml2musicxml___
#define ___musicxml2musicxml___

#include "libmusicxml.h" // for xmlErr


using namespace std;

namespace MusicXML2
{
/*
  The API functions with an options vector and no handler
  are declared in libmusicxml.h
*/

//_______________________________________________________________________________
EXP xmlErr musicxmlFile2musicxmlWithHandler (
  const char *  fileName,
  std::ostream& out,
  std::ostream& err,
  S_oahHandler  handler);

//_______________________________________________________________________________
EXP xmlErr musicxmlFd2musicxmlWithHandler (
  FILE *        fd,
  std::ostream& out,
  std::ostream& err,
  S_oahHandler  handler);

//_______________________________________________________________________________
EXP xmlErr musicxmlString2musicxmlWithHandler (
  const char *  buffer,
  std::ostream& out,
  std::ostream& err,
  S_oahHandler  handler);


}


#endif
