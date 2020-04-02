/*
  MusicXML Library
  Copyright (C) Grame 2006-2013

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr
*/

#include <climits>      // INT_MIN, INT_MAX

#include "msrTupletElements.h"

#include "msrOah.h"


using namespace std;

namespace MusicXML2
{

//______________________________________________________________________________
msrTupletElement::msrTupletElement (
  int inputLineNumber)
    : msrMeasureElement (inputLineNumber)
{
  fPositionInTuplet = -1;
}

msrTupletElement::~msrTupletElement ()
{}

ostream& operator<< (ostream& os, const S_msrTupletElement& elt)
{
  elt->print (os);
  return os;
}


}
