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

#include "xml.h"
#include "xmlfile.h"
#include "xmlreader.h"

#include "versions.h"

#include "waeMessagesHandling.h"

#include "enableTracingIfDesired.h"
#ifdef TRACING_IS_ENABLED
  #include "traceOah.h"
#endif

#include "oahOah.h"
#include "generalOah.h"

#include "mxmlTree2msrSkeletonBuilderInterface.h"

#include "mxmlTree2msrSkeletonBuilder.h"

#include "msr2summaryVisitor.h"

using namespace std;

namespace MusicXML2
{
//_______________________________________________________________________________
S_msrScore convertMxmlTreeToMsrScoreSkeleton (
  Sxmlelement    mxmlTree,
  S_msrOahGroup& msrOpts,
  string         passNumber,
  string         passDescription)
{
  // sanity check
  msgAssert (
    __FILE__, __LINE__,
    mxmlTree != nullptr,
    "convertMxmlTreeToMsrScoreSkeleton(): mxmlTree is null");

  // start the clock
  clock_t startClock = clock ();

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTracePasses ()) {
    string separator =
      "%--------------------------------------------------------------";

    gLogStream <<
      endl <<
      separator <<
      endl <<
      gTab <<
      passNumber << ": translating the mxmlElement tree into an MSR skeleton" <<
      endl;

    gLogStream <<
      separator <<
      endl;
  }
#endif

  S_msrScore scoreSkeleton;

  // create an mxmlTree2msrSkeletonBuilder
  mxmlTree2msrSkeletonBuilder
    skeletonBuilder;

  // build the MSR score
  skeletonBuilder.browseMxmlTree (
    mxmlTree);

  // fetch the score skeleton
  scoreSkeleton =
    skeletonBuilder.getMsrScore ();

  // register time spent
  clock_t endClock = clock ();

  timing::gGlobalTiming.appendTimingItem (
    passNumber,
    passDescription,
    timingItem::kMandatory,
    startClock,
    endClock);

  // check indentation
  if (gIndenter != 0) {
    if (! gGlobalGeneralOahGroup->getQuiet ()) {
      stringstream s;

      s <<
        "gIndenter value after " << passNumber << ": " <<
        gIndenter.getIndent ();

      msrMusicXMLWarning (
        gGlobalOahOahGroup->getInputSourceName (),
        1, // JMI inputLineNumber,
        s.str ());
    }

    gIndenter.resetToZero ();
  }


  if (! scoreSkeleton) {
    string message =
      "### Conversion from mxmlTree to an MSR skeleton failed ###";

    gLogStream <<
      message <<
      endl;

    throw msgMsrScoreToBsrScoreException (message);
  }

  if (msrOpts->getDisplayMsrSkeleton ()) {
    // Display the MSR skeleton
    displayMsrSkeleton (
      msrOpts,
      scoreSkeleton,
      passNumber,
      passDescription);
  }

  return scoreSkeleton;
}

//_______________________________________________________________________________
void displayMsrSkeleton (
  S_msrOahGroup&  msrOpts,
  S_msrScore theMsrScore,
  string     passNumber,
  string     passDescription)
{
  // sanity check
  msgAssert (
    __FILE__, __LINE__,
    theMsrScore != nullptr,
    "theMsrScore is null");

  // start the clock
  clock_t startClock = clock ();

  string separator =
    "%--------------------------------------------------------------";

  gLogStream <<
    endl <<
    separator <<
    endl <<
    gTab <<
    "Optional pass: displaying the MSR skeleton as text" <<
    endl <<
    separator <<
    endl << endl <<
    theMsrScore;

  // register time spent
  clock_t endClock = clock ();

  timing::gGlobalTiming.appendTimingItem (
    "",
    "Display the MSR skeleton as text",
    timingItem::kOptional,
    startClock,
    endClock);
}


}
