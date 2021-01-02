/*
  MusicXML Library
  Copyright (C) Grame 2006-2013

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr
*/

#include <iomanip>      // setw, setprecision, ...

#include "utilities.h"

#include "bsrMeasures.h"

#include "enableTracingIfDesired.h"
#ifdef TRACING_IS_ENABLED
  #include "traceOah.h"
#endif

#include "oahOah.h"

#include "bsrOah.h"
#include "brailleOah.h"


using namespace std;

namespace MusicXML2
{

//______________________________________________________________________________
S_bsrMeasure bsrMeasure::create (
  int    inputLineNumber,
  string printMeasureNumber)
{
  bsrMeasure* o =
    new bsrMeasure (
      inputLineNumber, printMeasureNumber);
  assert (o!=0);
  return o;
}

bsrMeasure::bsrMeasure (
  int    inputLineNumber,
  string printMeasureNumber)
    : bsrLineContentsElement (inputLineNumber)
{
  fPrintMeasureNumber = printMeasureNumber;

  // initially, fBrailleMeasureNumber is the same as fPrintMeasureNumber
  fBrailleMeasureNumber = fPrintMeasureNumber;

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceMeasures ()) {
    gLogStream <<
      "Creating bsrMeasure '" <<
      asString () <<
      "', line " <<
      fInputLineNumber <<
      endl;
  }
#endif
}

bsrMeasure::~bsrMeasure ()
{}

S_bsrMeasure bsrMeasure::createMeasureNewbornClone ()
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceMeasures ()) {
    gLogStream <<
      "Creating a newborn clone of measure " <<
      asString () <<
      endl;
  }
#endif

  S_bsrMeasure
    newbornClone =
      bsrMeasure::create (
        fInputLineNumber,
        fPrintMeasureNumber);

  // braille measure number
  newbornClone->fBrailleMeasureNumber =
    fBrailleMeasureNumber;

  return newbornClone;
}

void bsrMeasure::appendLineElementToMeasure (
  S_bsrLineContentsElement lineElement)
{
  fMeasureLineElementsList.push_back (lineElement);
}

void bsrMeasure::appendClefToMeasure (S_bsrClef clef)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceClefs () || gGlobalTraceOahGroup->getTraceMeasures ()) {
    gLogStream <<
      "Appending clef '" <<
      clef->asShortString () <<
      "' to measure '" <<
      asString () <<
      "'" <<
      endl;
    }
#endif

  appendLineElementToMeasure (clef);
}

void bsrMeasure::appendBarlineToMeasure (S_bsrBarline barline)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceBarlines () || gGlobalTraceOahGroup->getTraceMeasures ()) {
    gLogStream <<
      "Appending barline " <<
      barline->asShortString () <<
      " to measure '" <<
      asString () <<
      "'" <<
      endl;
    }
#endif

  appendLineElementToMeasure (barline);
}

void bsrMeasure::appendNumberToMeasure (S_bsrNumber number)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalBsrOahGroup->getTraceNumbers () || gGlobalTraceOahGroup->getTraceMeasures ()) {
    gLogStream <<
      "Appending number '" <<
      number->asShortString () <<
      "' to measure '" <<
      asString () <<
      "'" <<
      endl;
    }
#endif

  appendLineElementToMeasure (number);
}

void bsrMeasure::appendWordsToMeasure (S_bsrWords words)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalBsrOahGroup->getTraceNumbers () || gGlobalTraceOahGroup->getTraceMeasures ()) {
    gLogStream <<
      "Appending words '" <<
      words->asShortString () <<
      "' to measure '" <<
      asString () <<
      "'" <<
      endl;
    }
#endif

  appendLineElementToMeasure (words);
}

void bsrMeasure::appendNoteToMeasure (S_bsrNote note)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceNotes () || gGlobalTraceOahGroup->getTraceMeasures ()) {
    gLogStream <<
      "Appending note '" <<
      note->asShortString () <<
      "' to measure '" <<
      asString () <<
      "'" <<
      endl;
    }
#endif

  appendLineElementToMeasure (note);
}

void bsrMeasure::appendDynamicsToMeasure (S_bsrDynamics dynamics)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceDynamics () || gGlobalTraceOahGroup->getTraceMeasures ()) {
    gLogStream <<
      "Appending dynamics '" <<
      dynamics->asShortString () <<
      "' to measure '" <<
      asString () <<
      "'" <<
      endl;
    }
#endif

  appendLineElementToMeasure (dynamics);
}

S_bsrCellsList bsrMeasure::buildCellsList () const
{
  S_bsrCellsList
    result =
      bsrCellsList::create (fInputLineNumber);

  for (
    list<S_bsrLineContentsElement>::const_iterator i = fMeasureLineElementsList.begin ();
    i != fMeasureLineElementsList.end ();
    ++i ) {
    // append the braille for the element
    result->
      appendCellsListToCellsList (
        (*i)->fetchCellsList ());
  } // for

  return result;
}

int bsrMeasure::fetchCellsNumber () const
{
  return buildCellsList ()->fetchCellsNumber ();
}

void bsrMeasure::acceptIn (basevisitor* v)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalBsrOahGroup->getTraceBsrVisitors ()) {
    gLogStream <<
      "% ==> bsrMeasure::acceptIn ()" <<
      endl;
  }
#endif

  if (visitor<S_bsrMeasure>*
    p =
      dynamic_cast<visitor<S_bsrMeasure>*> (v)) {
        S_bsrMeasure elem = this;

#ifdef TRACING_IS_ENABLED
        if (gGlobalBsrOahGroup->getTraceBsrVisitors ()) {
          gLogStream <<
            "% ==> Launching bsrMeasure::visitStart ()" <<
            endl;
        }
#endif
        p->visitStart (elem);
  }
}

void bsrMeasure::acceptOut (basevisitor* v)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalBsrOahGroup->getTraceBsrVisitors ()) {
    gLogStream <<
      "% ==> bsrMeasure::acceptOut ()" <<
      endl;
  }
#endif

  if (visitor<S_bsrMeasure>*
    p =
      dynamic_cast<visitor<S_bsrMeasure>*> (v)) {
        S_bsrMeasure elem = this;

#ifdef TRACING_IS_ENABLED
        if (gGlobalBsrOahGroup->getTraceBsrVisitors ()) {
          gLogStream <<
            "% ==> Launching bsrMeasure::visitEnd ()" <<
            endl;
        }
#endif
        p->visitEnd (elem);
  }
}

void bsrMeasure::browseData (basevisitor* v)
{
  for (
    list<S_bsrLineContentsElement>::const_iterator i = fMeasureLineElementsList.begin ();
    i != fMeasureLineElementsList.end ();
    ++i ) {
    // browse the element
    bsrBrowser<bsrElement> browser (v);
    browser.browse (*(*i));
  } // for
}

string bsrMeasure::asString () const
{
  stringstream s;

  s <<
    "Measure" <<
    ", printMeasureNumber: " << fPrintMeasureNumber <<
    ", printMeasureNumber: " << fPrintMeasureNumber <<
    ", brailleMeasureNumber: " << fBrailleMeasureNumber <<
    ", measureElementsList.size (): " << fMeasureLineElementsList.size () <<
    ", spacesBefore: " << fSpacesBefore <<
    ", line " << fInputLineNumber;

  return s.str ();
}

string bsrMeasure::asDebugString () const // JMI
{
  stringstream s;

  s << "{";

  unsigned int measureElementsListSize = fMeasureLineElementsList.size ();

  if (measureElementsListSize) {
    list<S_bsrLineContentsElement>::const_iterator
      iBegin = fMeasureLineElementsList.begin (),
      iEnd   = fMeasureLineElementsList.end (),
      i      = iBegin;
    for ( ; ; ) {
      s << (*i)->asDebugString ();
      if (++i == iEnd) break;
      s << "|";
    } // for
  }

  s << "}";

  return s.str ();
}

void bsrMeasure::print (ostream& os) const
{
  os <<
    "Measure" <<
    ", printMeasureNumber " << fPrintMeasureNumber <<
    ", line " << fInputLineNumber <<
    endl;

  ++gIndenter;

  const unsigned int fieldWidth = 21;

  // print the measure numbers
  os << left <<
    setw (fieldWidth) <<
    "printMeasureNumber" << " : " << fPrintMeasureNumber <<
    endl <<
    setw (fieldWidth) <<
    "brailleMeasureNumber" << " : " << fBrailleMeasureNumber <<
    endl;

  // print the number of cells
  os << left <<
    setw (fieldWidth) <<
    "cellsNumber" << " : " << fetchCellsNumber () <<
    endl;

  // print spaces needs
  os << left <<
    setw (fieldWidth) <<
    "spacesBefore" << " : " <<
    fSpacesBefore <<
    endl;

  os << endl;

  // print the measure elements if any
  unsigned int measureElementsListSize = fMeasureLineElementsList.size ();

  if (measureElementsListSize || gGlobalBsrOahGroup->getDisplayBsrDetails ()) {
    os <<
//      setw (fieldWidth) <<
      "MeasureElementsList" <<
      ", " <<
      singularOrPlural (
        measureElementsListSize, "element", "elements");
    if (measureElementsListSize) {
      os << endl;
      ++gIndenter;

      list<S_bsrLineContentsElement>::const_iterator
        iBegin = fMeasureLineElementsList.begin (),
        iEnd   = fMeasureLineElementsList.end (),
        i      = iBegin;
      for ( ; ; ) {
        os << (*i);
        if (++i == iEnd) break;
        os << endl;
      } // for

      --gIndenter;
    }
    else {
      os <<
        " : " << "none" <<
      endl;
    }
  }

  --gIndenter;
}

ostream& operator<< (ostream& os, const S_bsrMeasure& elt)
{
  elt->print (os);
  return os;
}


}
