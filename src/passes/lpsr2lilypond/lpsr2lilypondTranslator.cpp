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
#include <cmath>
#include <string>

#include "enableTracingIfDesired.h"
#ifdef TRACING_IS_ENABLED
  #include "traceOah.h"
#endif

#include "oahOah.h"
#include "generalOah.h"

#include "mxmlTree2msrOah.h"
#include "lpsr2lilypondOah.h"

#include "lpsr2lilypondTranslator.h"


using namespace std;

namespace MusicXML2
{

// for comments highlighting in the generated Lilypond code
const int commentFieldWidth = 30;

//______________________________________________________________________________
S_lpsrRepeatDescr lpsrRepeatDescr::create (
  S_msrRepeat repeat,
  int         repeatEndingsNumber)
{
  lpsrRepeatDescr* o = new
    lpsrRepeatDescr (
      repeat,
      repeatEndingsNumber);
  assert (o!=0);
  return o;
}

lpsrRepeatDescr::lpsrRepeatDescr (
  S_msrRepeat repeat,
  int         repeatEndingsNumber)
{
  fRepeat = repeat;

  fRepeatEndingsNumber  = repeatEndingsNumber;
  fRepeatEndingsCounter = 0;

  fEndOfRepeatHasBeenGenerated = false;
}

lpsrRepeatDescr::~lpsrRepeatDescr ()
{}

string lpsrRepeatDescr::repeatDescrAsString () const
{
  stringstream s;

  s <<
    "fRepeatEndingsNumber = " <<
    fRepeatEndingsNumber <<
    ", fRepeatEndingsCounter = " <<
    fRepeatEndingsCounter <<
    ", fEndOfRepeatHasBeenGenerated = " <<
    booleanAsString (
      fEndOfRepeatHasBeenGenerated);

  return s.str ();
}

void lpsrRepeatDescr::print (ostream& os) const
{
  const int fieldWidth = 29;

  os << left <<
    setw (fieldWidth) <<
    "fRepeatEndingsNumber" << " : " <<
    fRepeatEndingsNumber <<
    endl <<
    setw (fieldWidth) <<
    "fRepeatEndingsCounter" << " : " <<
    fRepeatEndingsCounter <<
    endl <<
    setw (fieldWidth) <<
    "fEndOfRepeatHasBeenGenerated" << " : " <<
    fEndOfRepeatHasBeenGenerated <<
    endl;
}

ostream& operator<< (ostream& os, const S_lpsrRepeatDescr& elt)
{
  elt->print (os);
  return os;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::setCurrentOctaveEntryReferenceFromTheLilypondOah ()
{
  if (gGlobalLpsr2lilypondOahGroup->getRelativeOctaveEntrySemiTonesPitchAndOctave ()) {
    // option '-rel, -relative' has been used:
    fCurrentOctaveEntryReference =
      msrNote::createNoteFromSemiTonesPitchAndOctave (
        K_NO_INPUT_LINE_NUMBER,
        gGlobalLpsr2lilypondOahGroup->getRelativeOctaveEntrySemiTonesPitchAndOctave ());
  }
  else {
    fCurrentOctaveEntryReference = nullptr;
    // the first note in the voice will become the initial reference
  }

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceNotesOctaveEntry ()) {
    gLogStream <<
      "setCurrentOctaveEntryReferenceFromTheLilypondOah()" <<
      ", octaveEntryKind is" <<
      lpsrOctaveEntryKindAsString (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) <<
      endl <<
      "Initial fCurrentOctaveEntryReference is ";

    if (fCurrentOctaveEntryReference) {
      gLogStream <<
        "'" <<
        fCurrentOctaveEntryReference->asString () <<
        "'";
    }
    else {
      gLogStream << "none";
    }
    gLogStream << endl;
  }
#endif
}

//________________________________________________________________________
lpsr2lilypondTranslator::lpsr2lilypondTranslator (
  S_lpsrScore     lpsrScore,
  S_msrOahGroup&  msrOpts,
  S_lpsrOahGroup& lpsrOpts,
  ostream&        lilypondCodeStream)
  : fLilypondCodeStream (
      lilypondCodeStream)
{
  fMsrOahGroup  = msrOpts;
  fLpsrOahGroup = lpsrOpts;

  // the LPSR score we're visiting
  fVisitedLpsrScore = lpsrScore;

  // inhibit the browsing of grace notes groups before,
  // since they are handled at the note level
  fVisitedLpsrScore->
    getMsrScore ()->
      setInhibitGraceNotesGroupsBrowsing ();

  // inhibit the browsing of measures repeats replicas,
  // since Lilypond only needs the repeat measure
  fVisitedLpsrScore->
    getMsrScore ()->
      setInhibitMeasuresRepeatReplicasBrowsing ();

/* JMI
  // inhibit the browsing of measures repeat replicas,
  // since Lilypond only needs the measure number
  fVisitedLpsrScore->
    getMsrScore ()->
      setInhibitRestMeasuresBrowsing ();
*/

  // header handling
  fOnGoingHeader = false;

  // time
  fVoiceIsCurrentlySenzaMisura = false;
  fOnGoingVoiceCadenza = false;

  // staves
  fOnGoingStaff = false;

  // voices
  fOnGoingVoice = false;

  // octaves entry
  // ------------------------------------------------------
  /* initialize reference is:
        mobile in relative mode
        unused in absolute mode
        fixed  in fixed mode
  */
  switch (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) {
    case kOctaveEntryRelative:
      setCurrentOctaveEntryReferenceFromTheLilypondOah ();
      break;

    case kOctaveEntryAbsolute:
      fCurrentOctaveEntryReference = nullptr;
      break;

    case kOctaveEntryFixed:
      // sanity check
      msgAssert (
        gGlobalLpsr2lilypondOahGroup->getFixedOctaveEntrySemiTonesPitchAndOctave () != nullptr,
        "gGlobalLpsr2lilypondOahGroup->getFixedOctaveEntrySemiTonesPitchAndOctave () is null");

      fCurrentOctaveEntryReference =
        msrNote::createNoteFromSemiTonesPitchAndOctave (
          K_NO_INPUT_LINE_NUMBER,
          gGlobalLpsr2lilypondOahGroup->getFixedOctaveEntrySemiTonesPitchAndOctave ());
      break;
  } // switch

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceNotesOctaveEntry ()) {
    gLogStream <<
      "lpsr2lilypondTranslator()" <<
      ", octaveEntryKind is" <<
      lpsrOctaveEntryKindAsString (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) <<
      endl <<
      "Initial fCurrentOctaveEntryReference is ";

    if (fCurrentOctaveEntryReference) {
      gLogStream <<
        "'" <<
        fCurrentOctaveEntryReference->asString () <<
        "'";
    }
    else {
      gLogStream << "none";
    }
    gLogStream << endl;
  }
#endif

  // harmonies
  fOnGoingHarmonyVoice = false;
  fPowerChordHaveAlreadyBeenGenerated = false;

  // figured bass
  fOnGoingFiguredBassVoice = false;
  fCurrentFiguredBassFiguresCounter = 0;

  // repeats

  // rest measures
  fRemainingRestMeasuresNumber = 0;
  fOnGoingRestMeasures = false;

  // measures
  fCurrentVoiceMeasuresCounter = -1;

  // durations
  fLastMetWholeNotes = rational (0, 1);

  // notes
  fCurrentNotePrinObjectKind = kPrintObjectYes; // default value

  // grace notes
  fOnGoingGraceNotesGroup = false;
  fOnGoingChordGraceNotesGroupLink = false;

  // articulations
  fCurrentArpeggioDirectionKind = kDirectionNone;

  // stems
  fCurrentStemKind = msrStem::kStemNone;

  // double tremolos

  // chords
  fOnGoingChord = false;

  // trills
  fOnGoingTrillSpanner = false;

  // spanners
  fCurrentSpannerPlacementKind = kPlacementNone;

  // stanzas
  fGenerateCodeForOngoingNonEmptyStanza = false;

  // book blocks
  fOnGoingBookBlock = false;

  // book part blocks
  fOnGoingBookPartBlock = false;

  // score blocks
  fOnGoingScoreBlock = false;

  // part group blocks
  fNumberOfPartGroupBlocks = -1;
  fPartGroupBlocksCounter  = 0;

  // part blocks
  fNumberOfPartGroupBlockElements = -1;
  fPartGroupBlockElementsCounter  = 0;

  // staff blocks
  fNumberOfStaffBlocksElements = -1;
  fStaffBlocksCounter  = 0;
};

lpsr2lilypondTranslator::~lpsr2lilypondTranslator ()
{}

//________________________________________________________________________
void lpsr2lilypondTranslator::generateLilypondCodeFromLpsrScore ()
{
  if (fVisitedLpsrScore) {
    // browse a msrScore browser
    msrBrowser<lpsrScore> browser (this);
    browser.browse (*fVisitedLpsrScore);
  }
}

//________________________________________________________________________
string lpsr2lilypondTranslator::absoluteOctaveAsLilypondString (
  int absoluteOctave)
{
  string result;

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceNotes ()) {
    fLilypondCodeStream <<
      endl <<
      "%{ absoluteOctave = " << absoluteOctave << " %} " <<
      endl;
  }
#endif

  // generate LilyPond absolute octave
  switch (absoluteOctave) {
    case 0:
      result = ",,,";
      break;
    case 1:
      result = ",,";
      break;
    case 2:
      result = ",";
      break;
    case 3:
      result = "";
      break;
    case 4:
      result = "'";
      break;
    case 5:
      result = "''";
      break;
    case 6:
      result = "'''";
      break;
    case 7:
      result = "''''";
      break;
    case 8:
      result = "'''''";
      break;
    default:
      {
        /* JMI
        stringstream s;

        s <<
          "%{absolute octave " << absoluteOctave << "???%}";

        result = s.str ();
        */
      }
  } // switch

  return result;
}

//________________________________________________________________________
string lpsr2lilypondTranslator::alterationKindAsLilypondString (
  msrAlterationKind alterationKind)
{
  string result;

  switch (alterationKind) {
    case kTripleFlat:
      result = "TRIPLE-FLAT";
      break;
    case kDoubleFlat:
      result = "DOUBLE-FLAT";
      break;
    case kSesquiFlat:
      result = "THREE-Q-FLAT";
      break;
    case kFlat:
      result = "FLAT";
      break;
    case kSemiFlat:
      result = "SEMI-FLAT";
      break;
    case kNatural:
      result = "NATURAL";
      break;
    case kSemiSharp:
      result = "SEMI-SHARP";
      break;
    case kSharp:
      result = "SHARP";
      break;
    case kSesquiSharp:
      result = "THREE-Q-SHARP";
      break;
    case kDoubleSharp:
      result = "DOUBLE-SHARP";
      break;
    case kTripleSharp:
      result = "TRIPLE-SHARP";
      break;
    case k_NoAlteration:
      result = "alteration???";
      break;
  } // switch

  return result;
}

//________________________________________________________________________
string lpsr2lilypondTranslator::lilypondOctaveInRelativeEntryMode (
  S_msrNote note)
{
  int inputLineNumber =
    note->getInputLineNumber ();

  // generate LilyPond octave relative to fCurrentOctaveEntryReference

  // in MusicXML, octave number is 4 for the octave starting with middle C
  int noteAbsoluteOctave =
    note->getNoteOctave ();

  msrDiatonicPitchKind
    noteDiatonicPitchKind =
      note->
        noteDiatonicPitchKind (
          inputLineNumber);

  msrDiatonicPitchKind
    referenceDiatonicPitchKind =
      fCurrentOctaveEntryReference->
        noteDiatonicPitchKind (
          inputLineNumber);

  string
    referenceDiatonicPitchKindAsString =
      fCurrentOctaveEntryReference->
        noteDiatonicPitchKindAsString (
          inputLineNumber);

  int
    referenceAbsoluteOctave =
      fCurrentOctaveEntryReference->
        getNoteOctave ();

  /*
    If no octave changing mark is used on a pitch, its octave is calculated
    so that the interval with the previous note is less than a fifth.
    This interval is determined without considering accidentals.
  */

  int
    noteAboluteDiatonicOrdinal =
      noteAbsoluteOctave * 7
        +
      noteDiatonicPitchKind - kC,

    referenceAboluteDiatonicOrdinal =
      referenceAbsoluteOctave * 7
        +
      referenceDiatonicPitchKind - kC;

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceNotesOctaveEntry ()) {
    const int fieldWidth = 28;

    gLogStream << left <<
      "lilypondOctaveInRelativeEntryMode() 1" <<
      endl <<

      setw (fieldWidth) <<
      "% noteAboluteDiatonicOrdinal" <<
      " = " <<
      noteAboluteDiatonicOrdinal <<
      endl <<

      setw (fieldWidth) <<
      "% referenceDiatonicPitchAsString" <<
      " = " <<
      referenceDiatonicPitchKindAsString <<
      endl <<
      setw (fieldWidth) <<
      "% referenceAbsoluteOctave" <<
       " = " <<
      referenceAbsoluteOctave <<
      endl <<
      setw (fieldWidth) <<
      "% referenceAboluteDiatonicOrdinal" <<
      " = " <<
      referenceAboluteDiatonicOrdinal <<
      endl << endl;
  }
#endif

  stringstream s;

  // generate the octaves as needed
  if (noteAboluteDiatonicOrdinal >= referenceAboluteDiatonicOrdinal) {
    noteAboluteDiatonicOrdinal -= 4;
    while (noteAboluteDiatonicOrdinal >= referenceAboluteDiatonicOrdinal) {
      s << "'";
      noteAboluteDiatonicOrdinal -= 7;
    } // while
  }

  else {
    noteAboluteDiatonicOrdinal += 4;
    while (noteAboluteDiatonicOrdinal <= referenceAboluteDiatonicOrdinal) {
      s << ",";
      noteAboluteDiatonicOrdinal += 7;
    } // while
  }

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceNotesOctaveEntry ()) {
    gLogStream <<
      "lilypondOctaveInRelativeEntryMode() 2" <<
      ", result = " << s.str () <<
      endl << endl;
  }
#endif

  return s.str ();
}

//________________________________________________________________________
string lpsr2lilypondTranslator::lilypondOctaveInFixedEntryMode (
  S_msrNote note)
{
  // generate LilyPond octave relative to fCurrentOctaveEntryReference

  // in MusicXML, octave number is 4 for the octave starting with middle C

  /*
    Pitches in fixed octave entry mode only need commas or quotes
    when they are above or below fCurrentOctaveEntryReference,
    hence when their octave is different that the latter's.
  */

  int
    noteAbsoluteOctave =
      note->getNoteOctave ();

  int
    referenceAbsoluteOctave =
      fCurrentOctaveEntryReference->
        getNoteOctave ();

  int absoluteOctavesDifference =
    noteAbsoluteOctave - referenceAbsoluteOctave;

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceNotesOctaveEntry ()) {
    gLogStream << left <<
      "% noteAbsoluteOctave = " <<
      noteAbsoluteOctave <<
      ", referenceAbsoluteOctave = " <<
      referenceAbsoluteOctave <<
      ", referenceAbsoluteOctave = " <<
      absoluteOctavesDifference <<
      endl;
  }
#endif

  stringstream s;

  // generate the octaves as needed
  switch (absoluteOctavesDifference) {
    case -12:
      s << ",,,,,,,,,,,,";
      break;
    case -11:
      s << ",,,,,,,,,,,";
      break;
    case -10:
      s << ",,,,,,,,,,";
      break;
    case -9:
      s << ",,,,,,,,,";
      break;
    case -8:
      s << ",,,,,,,,";
      break;
    case -7:
      s << ",,,,,,,";
      break;
    case -6:
      s << ",,,,,,";
      break;
    case -5:
      s << ",,,,,";
      break;
    case -4:
      s << ",,,,";
      break;
    case -3:
      s << ",,,";
      break;
    case -2:
      s << ",,";
      break;
    case -1:
      s << ",";
      break;
    case 0:
      break;
    case 1:
      s << "'";
      break;
    case 2:
      s << "''";
      break;
    case 3:
      s << "'''";
      break;
    case 4:
      s << "''''";
      break;
    case 5:
      s << "'''''";
      break;
    case 6:
      s << "''''''";
      break;
    case 7:
      s << "'''''''";
      break;
    case 8:
      s << "''''''''";
      break;
    case 9:
      s << "'''''''''";
      break;
    case 10:
      s << "''''''''''";
      break;
    case 11:
      s << "'''''''''''";
      break;
    case 12:
      s << "''''''''''''";
      break;
    default:
      s << "!!!";
  } // switch

  return s.str ();
}

//________________________________________________________________________
string lpsr2lilypondTranslator::stringTuningAsLilypondString (
  int               inputLineNumber,
  S_msrStringTuning stringTuning)
{
  msrDiatonicPitchKind
    stringTuningDiatonicPitchKind =
      stringTuning->
        getStringTuningDiatonicPitchKind ();

  msrAlterationKind
    stringTuningAlterationKind =
      stringTuning->
       getStringTuningAlterationKind ();

  int
    stringTuningOctave =
      stringTuning->
       getStringTuningOctave ();

  // compute the quartertones pitch
  msrQuarterTonesPitchKind
    quarterTonesPitchKind =
      quarterTonesPitchKindFromDiatonicPitchAndAlteration (
        inputLineNumber,
        stringTuningDiatonicPitchKind,
        stringTuningAlterationKind);

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceScordaturas ()) {
    int
      getStringTuningNumber =
        stringTuning->
          getStringTuningNumber ();

    gLogStream <<
      endl <<
      "%getStringTuningNumber = " <<
      getStringTuningNumber <<
      endl <<
      "%stringTuningDiatonicPitchKind = " <<
      msrDiatonicPitchKindAsString (
        stringTuningDiatonicPitchKind) <<
      endl <<
      "%stringTuningAlterationKind = " <<
      alterationKindAsLilypondString (
        stringTuningAlterationKind) <<
      endl <<
      "%stringTuningOctave = " <<
      stringTuningOctave <<
      endl <<
      "%quarterTonesPitchKind = " <<
      quarterTonesPitchKind <<
      endl <<
      "%quarterTonesPitchKindAsString: " <<
      msrQuarterTonesPitchKindAsString (
        gGlobalLpsrOahGroup->
          getLpsrQuarterTonesPitchesLanguageKind (),
          quarterTonesPitchKind) <<
      endl << endl;
  }
#endif

  stringstream s;

  s <<
    msrQuarterTonesPitchKindAsString (
      gGlobalLpsrOahGroup->
        getLpsrQuarterTonesPitchesLanguageKind (),
        quarterTonesPitchKind) <<
    absoluteOctaveAsLilypondString (
      stringTuningOctave);

  return s.str ();
}

//________________________________________________________________________
string lpsr2lilypondTranslator::notePitchAsLilypondString (
  S_msrNote note)
{
  stringstream s;

  // should an editorial accidental be generated?
  switch (note->getNoteEditorialAccidentalKind ()) {
    case kEditorialAccidentalYes:
      s <<
        "\\editorialAccidental ";
      break;
    case kEditorialAccidentalNo:
      break;
  } // switch

  // get the note quarter tones pitch
  msrQuarterTonesPitchKind
    noteQuarterTonesPitchKind =
      note->
        getNoteQuarterTonesPitchKind ();

  // fetch the quarter tones pitch as string
  string
    quarterTonesPitchKindAsString =
      msrQuarterTonesPitchKindAsString (
        gGlobalLpsrOahGroup->getLpsrQuarterTonesPitchesLanguageKind (),
        noteQuarterTonesPitchKind);

  // get the note quarter tones display pitch
  msrQuarterTonesPitchKind
    noteQuarterTonesDisplayPitchKind =
      note->
        getNoteQuarterTonesDisplayPitchKind ();

  // fetch the quarter tones display pitch as string
  string
    quarterTonesDisplayPitchKindAsString =
      msrQuarterTonesPitchKindAsString (
        gGlobalLpsrOahGroup->getLpsrQuarterTonesPitchesLanguageKind (),
        noteQuarterTonesDisplayPitchKind);

  // generate the pitch
  s <<
    quarterTonesPitchKindAsString;

  // in MusicXML, octave number is 4 for the octave
  // starting with middle C, LilyPond's c'
  int
    noteAbsoluteOctave =
      note->getNoteOctave ();

  // should an absolute octave be generated?
#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceNotesOctaveEntry ()) {
    int
      noteAbsoluteDisplayOctave =
        note->getNoteDisplayOctave ();

    const int fieldWidth = 39;

    gLogStream << left <<
      "notePitchAsLilypondString() 1" <<
      endl <<

      setw (fieldWidth) <<
      "% quarterTonesPitchKindAsString" <<
      " = " <<
      quarterTonesPitchKindAsString <<
      endl <<
      setw (fieldWidth) <<
      "% quarterTonesDisplayPitchKindAsString" <<
      " = " <<
      quarterTonesDisplayPitchKindAsString <<
      endl <<

      setw (fieldWidth) <<
      "% noteAbsoluteOctave" <<
      " = " <<
      noteAbsoluteOctave <<
      endl <<
      setw (fieldWidth) <<
      "% noteAbsoluteDisplayOctave" <<
      " = " <<
      noteAbsoluteDisplayOctave <<
      endl <<

      setw (fieldWidth) <<
      "% line" <<
      " = " <<
      note->getInputLineNumber () <<
      endl;
  }
#endif

  switch (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) {
    case kOctaveEntryRelative:
      if (! fCurrentOctaveEntryReference) {
#ifdef TRACING_IS_ENABLED
        if (gGlobalTraceOahGroup->getTraceNotesOctaveEntry ()) {
          gLogStream <<
            "notePitchAsLilypondString() 2: fCurrentOctaveEntryReference is null" <<
            " upon note " << note->asString () <<
            ", line " << note->getInputLineNumber () <<
            endl;
        }
#endif

        // generate absolute octave
        s <<
          absoluteOctaveAsLilypondString (
            noteAbsoluteOctave);

        // fCurrentOctaveEntryReference will be set to note later
      }
      else {
#ifdef TRACING_IS_ENABLED
        if (gGlobalTraceOahGroup->getTraceNotesOctaveEntry ()) {
          gLogStream <<
            "notePitchAsLilypondString() 3: fCurrentOctaveEntryReference is '" <<
            fCurrentOctaveEntryReference->asString () <<
            "' upon note " << note->asString () <<
            ", line " << note->getInputLineNumber () <<
            endl;
        }
#endif

        // generate octave relative to mobile fCurrentOctaveEntryReference
        s <<
          lilypondOctaveInRelativeEntryMode (note);
      }
      break;

    case kOctaveEntryAbsolute:
      // generate LilyPond absolute octave
      s <<
        absoluteOctaveAsLilypondString (
          noteAbsoluteOctave);
      break;

    case kOctaveEntryFixed:
      // generate octave relative to fixed fCurrentOctaveEntryReference
      s <<
        lilypondOctaveInFixedEntryMode (note);
      break;
  } // switch

  // should an accidental be generated? JMI this can be fine tuned with cautionary
  switch (note->getNoteAccidentalKind ()) {
    case kAccidentalNone:
      break;
    default:
      s <<
        "!";
      break;
  } // switch

  // should an editorial accidental be generated?
  switch (note->getNoteEditorialAccidentalKind ()) {
    case kEditorialAccidentalYes:
      s <<
        "?";
      break;
    case kEditorialAccidentalNo:
      break;
  } // switch

  // should a cautionary accidental be generated?
  switch (note->getNoteCautionaryAccidentalKind ()) {
    case kCautionaryAccidentalYes:
      s <<
        "?";
      break;
    case kCautionaryAccidentalNo:
      break;
  } // switch

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceNotesOctaveEntry ()) {
    gLogStream << endl;
  }
#endif

  return s.str ();
}

//________________________________________________________________________
string lpsr2lilypondTranslator::durationAsLilypondString (
  int      inputLineNumber,
  rational wholeNotes)
{
  string result;

  bool generateExplicitDuration;

  if (wholeNotes != fLastMetWholeNotes) {
    generateExplicitDuration = true;
    fLastMetWholeNotes = wholeNotes;
  }
  else {
    generateExplicitDuration =
      gGlobalLpsr2lilypondOahGroup->getAllDurations ();
  }

  if (generateExplicitDuration) {
    result =
      wholeNotesAsLilypondString (
        inputLineNumber,
        wholeNotes);
  }

  return result;
}

//________________________________________________________________________
string lpsr2lilypondTranslator::pitchedRestAsLilypondString (
  S_msrNote note)
{
  stringstream s;

  // get the note quarter tones pitch
  msrQuarterTonesPitchKind
    noteQuarterTonesPitchKind =
      note->
        getNoteQuarterTonesPitchKind ();

  // fetch the quarter tones pitch as string
  string
    noteQuarterTonesPitchKindAsString =
      msrQuarterTonesPitchKindAsString (
        gGlobalLpsrOahGroup->getLpsrQuarterTonesPitchesLanguageKind (),
        noteQuarterTonesPitchKind);

  // get the note quarter tones display pitch
  msrQuarterTonesPitchKind
    noteQuarterTonesDisplayPitchKind =
      note->
        getNoteQuarterTonesDisplayPitchKind ();

  // fetch the quarter tones display pitch as string
  string
    quarterTonesDisplayPitchKindAsString =
      msrQuarterTonesPitchKindAsString (
        gGlobalLpsrOahGroup->getLpsrQuarterTonesPitchesLanguageKind (),
        noteQuarterTonesDisplayPitchKind);

  // generate the display pitch
  s <<
    note->
      noteDisplayPitchKindAsString ();
//    note->notePitchAsString (); JMI
//    quarterTonesDisplayPitchAsString;

  // should an absolute octave be generated?
  int
    noteAbsoluteDisplayOctave =
      note->getNoteDisplayOctave ();

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceNotes ()) {
    // in MusicXML, octave number is 4 for the octave starting with middle C
    int noteAbsoluteOctave =
      note->getNoteOctave ();

    const int fieldWidth = 28;

    gLogStream << left <<
      "pitchedRestAsLilypondString()" <<
      endl <<

      setw (fieldWidth) <<
      "% noteQuarterTonesPitchKindAsString" <<
      " = " <<
      noteQuarterTonesPitchKindAsString <<
      endl <<
      setw (fieldWidth) <<
      "% quarterTonesDisplayPitch" <<
      " = " <<
      quarterTonesDisplayPitchKindAsString <<
      endl <<

      setw (fieldWidth) <<
      "% noteAbsoluteOctave" <<
      " = " <<
      noteAbsoluteOctave <<
      endl <<
      setw (fieldWidth) <<
      "% noteAbsoluteDisplayOctave" <<
      " = " <<
      noteAbsoluteDisplayOctave <<
      endl <<

      setw (fieldWidth) <<
      "% line" <<
      " = " <<
      note->getInputLineNumber () <<
      endl;
  }
#endif

  switch (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) {
    case kOctaveEntryRelative:
    // generate LilyPond octave relative to fCurrentOctaveEntryReference
    s <<
      lilypondOctaveInRelativeEntryMode (note);
      break;
    case kOctaveEntryAbsolute:
      // generate LilyPond absolute octave
      s <<
        absoluteOctaveAsLilypondString (
          noteAbsoluteDisplayOctave);
      break;
    case kOctaveEntryFixed:
      // generate LilyPond octave relative to fCurrentOctaveEntryReference
      s <<
        lilypondOctaveInFixedEntryMode (note);
      break;
  } // switch

  // generate the skip duration
  s <<
    durationAsLilypondString (
      note->getInputLineNumber (),
      note->
        getNoteSoundingWholeNotes ());

  // generate the '\rest'
  s <<
    "\\rest ";

  return s.str ();
}

void lpsr2lilypondTranslator::generateNoteHeadColor (
  S_msrNote note)
{
  int inputLineNumber =
    note->getInputLineNumber ();

  // has the note alpha RGB color been set?
 if (note->getNoteAlphaRGBColorHasBenSet ()) {
    // get note alpha RGB color
    msrAlphaRGBColor
      noteAlphaRGBColor =
        note->getNoteAlphaRGBColor ();

    // get note RGB color
    string noteRGB = noteAlphaRGBColor.getColorRGB ();

    // generate code for RGB color
    if (noteRGB.size () == 6) {
      string
        noteR = noteRGB.substr (0, 2),
        noteG = noteRGB.substr (2, 2),
        noteB = noteRGB.substr (4, 2);

      fLilypondCodeStream <<
        "\\once \\override NoteHead.color = #(map (lambda (x) (/ x 255)) "
        "'(" <<
        "#X" << noteRGB [0] << noteRGB [1] <<
        " " <<
        "#X" << noteRGB [2] << noteRGB [3] <<
        " " <<
        "#X" << noteRGB [4] << noteRGB [5] <<
        "))" <<
        endl;
      }
    else {
      stringstream s;

      s <<
        "note RGB color '" <<
        noteRGB <<
        "' is ill-formed" <<
        ", line " << inputLineNumber;

      msrInternalError (
        gGlobalOahOahGroup->getInputSourceName (),
        inputLineNumber,
        __FILE__, __LINE__,
        s.str ());
    }
  }
}

void lpsr2lilypondTranslator::generateNoteLigatures (
  S_msrNote note)
{
  list<S_msrLigature>
    noteLigatures =
      note->getNoteLigatures ();

  if (noteLigatures.size ()) {
    list<S_msrLigature>::const_iterator i;
    for (
      i=noteLigatures.begin ();
      i!=noteLigatures.end ();
      i++
    ) {
      S_msrLigature ligature = (*i);

      switch (ligature->getLigatureKind ()) {
        case msrLigature::kLigatureNone:
          break;

        case msrLigature::kLigatureStart:
          {
            /*
              the edge height is relative to the voice,
              i.e. a positive value points down in voices 1 and 3
              and it points up in voices 2 and 4,
            */

            // fetch note's voice
            S_msrVoice
              noteVoice =
                note->
                  fetchNoteVoiceUpLink ();

            // determine vertical flipping factor
            int ligatureVerticalFlippingFactor = 0;

            switch (noteVoice->getRegularVoiceStaffSequentialNumber ()) {
              case 1:
              case 3:
                ligatureVerticalFlippingFactor = 1;
                break;
              case 2:
              case 4:
                ligatureVerticalFlippingFactor = -1;
                break;
              default:
                ;
            } // switch

#ifdef TRACING_IS_ENABLED
            if (gGlobalTraceOahGroup->getTraceLigatures ()) {
              gLogStream <<
                "Ligature vertical flipping factore for note '" <<
                note->asString () <<
                "' in voice \"" <<
                noteVoice->getVoiceName () <<
                "\" is " <<
                ligatureVerticalFlippingFactor <<
                ", line " << ligature->getInputLineNumber () <<
                endl;
            }
#endif

            // compute ligature start edge height
            const float edgeHeightAbsValue = 0.75;

            float       ligatureStartEdgeHeight = 0.0;

            switch (ligature->getLigatureLineEndKind ()) {
              case msrLigature::kLigatureLineEndUp:
                ligatureStartEdgeHeight =
                  - ligatureVerticalFlippingFactor * edgeHeightAbsValue;
                break;

              case msrLigature::kLigatureLineEndDown:
                ligatureStartEdgeHeight =
                  ligatureVerticalFlippingFactor * edgeHeightAbsValue;
                break;

              case msrLigature::kLigatureLineEndBoth: // JMI
                ligatureStartEdgeHeight =
                  - ligatureVerticalFlippingFactor * edgeHeightAbsValue;
                break;

              case msrLigature::kLigatureLineEndArrow: // JMI
                fLilypondCodeStream <<
                  "%{ligatureLineEndArrow???%} ";
                break;

              case msrLigature::kLigatureLineEndNone:
                ligatureStartEdgeHeight = 0;
                break;
            } // switch

            // fetch ligature's other end
            S_msrLigature
              ligatureOtherEnd =
                ligature->
                  getLigatureOtherEndSideLink ();

            // compute ligature end edge height
            float ligatureEndEdgeHeight = 0.0;

            switch (ligatureOtherEnd->getLigatureLineEndKind ()) {
              case msrLigature::kLigatureLineEndUp:
                ligatureEndEdgeHeight =
                  - ligatureVerticalFlippingFactor * edgeHeightAbsValue;
                break;

              case msrLigature::kLigatureLineEndDown:
                ligatureEndEdgeHeight =
                  ligatureVerticalFlippingFactor * edgeHeightAbsValue;
                break;

              case msrLigature::kLigatureLineEndBoth: // JMI
                ligatureEndEdgeHeight =
                  - ligatureVerticalFlippingFactor * edgeHeightAbsValue;
                break;

              case msrLigature::kLigatureLineEndArrow: // JMI
                fLilypondCodeStream <<
                  "%{ligatureLineEndArrow???%} ";
                break;

              case msrLigature::kLigatureLineEndNone:
                ligatureEndEdgeHeight = 0;
                break;
            } // switch

            // generate the code the the edge-height pair of values
            fLilypondCodeStream <<
              endl <<
              "\\once \\override Staff.LigatureBracket.edge-height = #'(" <<
              setprecision (2) <<
              ligatureStartEdgeHeight <<
              " . " <<
              setprecision (2) <<
              ligatureEndEdgeHeight <<
              ")" <<
              " %{ " <<
              ligature->getInputLineNumber () <<
              " %}" <<
              endl;
          }

          // generate ligature line type if any
          switch (ligature->getLigatureLineTypeKind ()) {
            case kLineTypeSolid:
              break;
            case kLineTypeDashed:
              fLilypondCodeStream <<
                endl <<
                "\\once\\override LigatureBracket.style = #'dashed-line" <<
                endl;
              break;
            case kLineTypeDotted:
              fLilypondCodeStream <<
                endl <<
                "\\once\\override LigatureBracket.style = #'dotted-line" <<
                endl;
              break;
            case kLineTypeWavy:
              fLilypondCodeStream <<
                endl <<
                "\\once\\override LigatureBracket.style = #'zigzag" <<
                endl;
              break;
          } // switch

          fLilypondCodeStream << "\\[ ";
          break;

        case msrLigature::kLigatureContinue:
          break;

        case msrLigature::kLigatureStop:
   // JMI       fLilypondCodeStream << "\\] ";
          break;
      } // switch
    } // for
  }
}

string lpsr2lilypondTranslator::stemAsLilypondString (
  msrStem::msrStemKind stemKind)
{
  string result;

  switch (stemKind) {
    case msrStem::kStemNone:
      result = "\\stemNeutral ";
      break;
    case msrStem::kStemUp:
      result = "\\stemUp ";
      break;
    case msrStem::kStemDown:
      result = "\\stemDown ";
      break;
    case msrStem::kStemDouble: // JMI ???
      break;
  } // switch

  return result;
}

void lpsr2lilypondTranslator::generateNoteStemIfNeeded (
  S_msrStem stem)
{
  // the LilyPond \stem* commands have a persistent effect,
  // hence the use of fCurrentStemKind

  if (stem) {
    msrStem::msrStemKind
      stemKind =
        stem->getStemKind ();

    // should a stem direction command be generated?
    if (stemKind != fCurrentStemKind) {
      fLilypondCodeStream <<
        stemAsLilypondString (stemKind);

      fCurrentStemKind = stemKind;
    }
  }

  else {
//    msrStem::msrStemKind noteStemKind = msrStem::kStemNone;  // JMI
  }
}

void lpsr2lilypondTranslator::generateNoteHead (
  S_msrNote note)
{
  if (! note->getNoteIsARest ()) { // JMI ???
    msrNote::msrNoteHeadKind
      noteHeadKind =
        note->getNoteHeadKind ();

    // these tweaks should occur right before the note itself
    switch (noteHeadKind) {
      case msrNote::kNoteHeadSlash:
        fLilypondCodeStream << "\\tweak style #'slash ";
        break;
      case msrNote::kNoteHeadTriangle:
        fLilypondCodeStream << "\\tweak style #'triangle ";
        break;
      case msrNote::kNoteHeadDiamond:
   // JMI     fLilypondCodeStream << "\\tweak style #'diamond ";
        fLilypondCodeStream << "\\harmonic ";
        break;
      case msrNote::kNoteHeadSquare:
        fLilypondCodeStream << "\\tweak style #'la ";
        break;
      case msrNote::kNoteHeadCross:
        fLilypondCodeStream << "\\tweak style #'cross ";
        break;
      case msrNote::kNoteHeadX:
        fLilypondCodeStream << "\\tweak style #'cross %{x%} ";
        break;
      case msrNote::kNoteHeadCircleX:
        fLilypondCodeStream << "\\tweak style #'xcircle ";
        break;
      case msrNote::kNoteHeadInvertedTriangle:
        fLilypondCodeStream << "%{kNoteHeadInvertedTriangle%} ";
        break;
      case msrNote::kNoteHeadArrowDown:
        fLilypondCodeStream << "%{kNoteHeadArrowDown%} ";
        break;
      case msrNote::kNoteHeadArrowUp:
        fLilypondCodeStream << "%{kNoteHeadArrowUp%} ";
        break;
      case msrNote::kNoteHeadSlashed:
        fLilypondCodeStream << "%{kNoteHeadSlashed%} ";
        break;
      case msrNote::kNoteHeadBackSlashed:
        fLilypondCodeStream << "%{kNoteHeadBackSlashed%} ";
        break;
      case msrNote::kNoteHeadNormal:
   // JMI     fLilypondCodeStream << "%{kNoteHeadNormal%} ";
        break;
      case msrNote::kNoteHeadCluster:
        fLilypondCodeStream << "%{kNoteHeadCluster%} ";
        break;
      case msrNote::kNoteHeadCircleDot:
        fLilypondCodeStream << "%{kNoteHeadCircleDot%} ";
        break;
      case msrNote::kNoteHeadLeftTriangle:
        fLilypondCodeStream << "%{kNoteHeadLeftTriangle%} ";
        break;
      case msrNote::kNoteHeadRectangle:
        fLilypondCodeStream << "%{kNoteHeadRectangle%} ";
        break;
      case msrNote::kNoteHeadNone:
        fLilypondCodeStream << "\\once\\omit NoteHead ";
        break;
      case msrNote::kNoteHeadDo:
        fLilypondCodeStream << "\\tweak style #'do ";
        break;
      case msrNote::kNoteHeadRe:
        fLilypondCodeStream << "\\tweak style #'re ";
        break;
      case msrNote::kNoteHeadMi:
        fLilypondCodeStream << "\\tweak style #'mi ";
        break;
      case msrNote::kNoteHeadFa:
        fLilypondCodeStream << "\\tweak style #'fa ";
        break;
      case msrNote::kNoteHeadFaUp:
        fLilypondCodeStream << "\\tweak style #'triangle ";
        break;
      case msrNote::kNoteHeadSo:
        fLilypondCodeStream << "\\tweak style #'sol ";
        break;
      case msrNote::kNoteHeadLa:
        fLilypondCodeStream << "\\tweak style #'la ";
        break;
      case msrNote::kNoteHeadTi:
        fLilypondCodeStream << "\\tweak style #'ti ";
        break;
    } // switch
  }
}

void lpsr2lilypondTranslator::generateCoda (S_msrCoda coda)
{
  switch (coda->getCodaKind ()) {
    case msrCoda::kCodaFirst:
      // generate the coda
      fLilypondCodeStream <<
        "\\mark \\markup { \\musicglyph #\"scripts.coda\" }%{first%}";
      break;
    case msrCoda::kCodaSecond:
      // generate the coda
      fLilypondCodeStream <<
        "\\mark \\markup { \\musicglyph #\"scripts.coda\" }%{second%}";
      break;
  } // switch
  fLilypondCodeStream << endl;
}

void lpsr2lilypondTranslator::generateSegno (S_msrSegno segno)
{
  // generate the segno
  fLilypondCodeStream <<
    "\\mark \\markup { \\musicglyph #\"scripts.segno\" }" <<
    endl;
}

void lpsr2lilypondTranslator::generateCodeRightBeforeNote (S_msrNote note)
{
  if (! fOnGoingChord) {
    // generate the note codas if any
    const list<S_msrCoda>&
      noteCodas =
        note->getNoteCodas ();

    if (noteCodas.size ()) {
      list<S_msrCoda>::const_iterator i;
      for (i=noteCodas.begin (); i!=noteCodas.end (); i++) {
        S_msrCoda coda = (*i);

        // generate only the second coda before the note
        switch (coda->getCodaKind ()) {
          case msrCoda::kCodaFirst:
            break;
          case msrCoda::kCodaSecond:
            // generate the coda
            generateCoda (coda);
            break;
        } // switch
      } // for
    }

    // generate the note segnos if any
    const list<S_msrSegno>&
      noteSegnos =
        note->getNoteSegnos ();

    if (noteSegnos.size ()) {
      list<S_msrSegno>::const_iterator i;
      for (i=noteSegnos.begin (); i!=noteSegnos.end (); i++) {
      } // for
    }
  }

  // print the note head color
  generateNoteHeadColor (note);

  // print the note ligatures if any
  list<S_msrLigature>
    noteLigatures =
      note->getNoteLigatures ();

  if (noteLigatures.size ()) {
    generateNoteLigatures (note);
  }

  // print note stem kind if needed
  if (! fOnGoingChord) {
    S_msrStem
      noteStem =
        note->getNoteStem ();

    generateNoteStemIfNeeded (noteStem);
  }

  // handling note head
  generateNoteHead (note);
}

void lpsr2lilypondTranslator::generateCodeForNote (
  S_msrNote note)
{
  int inputLineNumber =
    note->getInputLineNumber ();

  ////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////
  // print the note itself
  ////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////

  switch (note->getNoteKind ()) {

    case msrNote::k_NoNoteKind:
      break;

    case msrNote::kRestNote:
      {
        // get pitched rest status
        bool noteIsAPitchedRest =
          note->noteIsAPitchedRest ();

        if (noteIsAPitchedRest) {
          // pitched rest
          fLilypondCodeStream <<
            pitchedRestAsLilypondString (note);

          // this note is the new relative octave reference
          // (the display quarter tone pitch and octave
          // have been copied to the note octave
          // in the msrNote::msrNote () constructor,
          // since the note octave is used in relative code generation)
          switch (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) {
            case kOctaveEntryRelative:
              fCurrentOctaveEntryReference = note;
              break;
            case kOctaveEntryAbsolute:
              break;
            case kOctaveEntryFixed:
              break;
          } // switch
        }

        else {
          // unpitched rest
          // get the note sounding whole notes
          rational
            noteSoundingWholeNotes =
              note->getNoteSoundingWholeNotes ();

          // get note's voice
          S_msrVoice
            noteVoice =
              note->
                fetchNoteVoiceUpLink ();

          // print the rest name and duration
          if (note->getNoteOccupiesAFullMeasure ()) {
            // take voice kind into account JMI shouldn't be necessary?
            switch (noteVoice->getVoiceKind ()) {
              case msrVoice::kVoiceRegular:
                fLilypondCodeStream <<
                  "R%{1%}";
                break;

              case msrVoice::kVoiceHarmony:
              case msrVoice::kVoiceFiguredBass:
                fLilypondCodeStream <<
                  "s%{1%}";
                break;
            } // switch

            fLilypondCodeStream <<
              durationAsLilypondString (
                inputLineNumber,
                noteSoundingWholeNotes);
          }

          else {
            // take voice kind into account JMI shouldn't be necessary?
            switch (noteVoice->getVoiceKind ()) {
              case msrVoice::kVoiceRegular:
                fLilypondCodeStream <<
                  "r%{2%}";
                break;

              case msrVoice::kVoiceHarmony:
              case msrVoice::kVoiceFiguredBass:
                fLilypondCodeStream <<
                  "s%{2%}";
                break;
            } // switch

            fLilypondCodeStream <<
              durationAsLilypondString (
                inputLineNumber,
                noteSoundingWholeNotes);

/* JMI BOF
            if (fOnGoingVoiceCadenza) { // JMI
              if (noteSoundingWholeNotes != rational (1, 1)) {
                / * JMI
                // force the generation of the duration if needed
                if (! gGlobalLpsr2lilypondOahGroup->getAllDurations ()) {
                  fLilypondCodeStream << // JMI
                    wholeNotesAsLilypondString (
                      inputLineNumber,
                      noteSoundingWholeNotes);
                }
                * /

                // generate the multiplying factor
                fLilypondCodeStream << // JMI
                  "*" <<
                  noteSoundingWholeNotes <<
                  "";
              }
            }
                  */
          }

          // an unpitched rest is no relative octave reference,
          // the preceding one is kept
        }
      }
      break;

    case msrNote::kSkipNote:
      if (gGlobalLpsr2lilypondOahGroup->getGeneratePositionsInMeasures ()) {
        // print the rest name to help pin-point bugs
        fLilypondCodeStream << "r%{3%}";
      }
      else {
        // print the skip name
        fLilypondCodeStream << "s%{44%}";
      }

      // print the skip duration
      fLilypondCodeStream <<
        durationAsLilypondString (
          inputLineNumber,
          note->
            getNoteSoundingWholeNotes ());

      // a skip is no relative octave reference,
      // the preceding one is kept
      break;

    case msrNote::kUnpitchedNote:
      {
        // print the note name, "e" by convention
        fLilypondCodeStream <<
            "e";

        rational
          noteSoundingWholeNotes =
            note->
              getNoteSoundingWholeNotes ();

        // print the note duration
        fLilypondCodeStream <<
          durationAsLilypondString (
            inputLineNumber,
            noteSoundingWholeNotes);

        // handle delayed ornaments if any
        if (note->getNoteDelayedTurnOrnament ()) {
          // c2*2/3 ( s2*1/3\turn) JMI
          // we need the explicit duration in all cases,
          // regardless of gGlobalGeneralOahGroup->getAllDurations ()
          fLilypondCodeStream <<
            wholeNotesAsLilypondString (
              inputLineNumber,
              noteSoundingWholeNotes) <<
            "*" <<
            gGlobalLpsr2lilypondOahGroup->getDelayedOrnamentsFraction ();
        }

/* JMI
        // print the tie if any
        {
          S_msrTie noteTie = note->getNoteTie ();

          if (noteTie) {
            if (noteTie->getTieKind () == msrTie::kTieStart) {
              fLilypondCodeStream <<
                "%{line " << inputLineNumber << "%}" <<
                " ~  %{kUnpitchedNote%}"; // JMI spaces???
            }
          }
        }
        */
      }
      break;

    case msrNote::kRegularNote:
      {
        // print the note name
        fLilypondCodeStream <<
          notePitchAsLilypondString (note);

        rational
          noteSoundingWholeNotes =
            note->
              getNoteSoundingWholeNotes ();

        // print the note duration
        fLilypondCodeStream <<
          durationAsLilypondString (
            inputLineNumber,
            noteSoundingWholeNotes);

        // handle delayed ornaments if any
        if (note->getNoteDelayedTurnOrnament ()) {
          // c2*2/3 ( s2*1/3\turn) JMI
          // we need the explicit duration in all cases,
          // regardless of gGlobalGeneralOahGroup->getAllDurations ()
          fLilypondCodeStream <<
          //* JMI TOO MUCH
            wholeNotesAsLilypondString (
              inputLineNumber,
              noteSoundingWholeNotes) <<
              //*/
            "*" <<
            gGlobalLpsr2lilypondOahGroup->getDelayedOrnamentsFraction ();
        }

        // print the tie if any
        {
          S_msrTie noteTie = note->getNoteTie ();

          if (noteTie) {
            if (noteTie->getTieKind () == msrTie::kTieStart) {
      //        fLilypondCodeStream << " ~ %{kRegularNote%}"; // JMI
            }
          }
        }

        // this note is the new relative octave reference
        switch (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) {
          case kOctaveEntryRelative:
            fCurrentOctaveEntryReference = note;
            break;
          case kOctaveEntryAbsolute:
            break;
          case kOctaveEntryFixed:
            break;
        } // switch
      }
      break;

    case msrNote::kDoubleTremoloMemberNote:
      // print the note name
      fLilypondCodeStream <<
        notePitchAsLilypondString (note);

      // print the note duration
      fLilypondCodeStream <<
        durationAsLilypondString (
          inputLineNumber,
          note->getNoteSoundingWholeNotes ());

      // handle delayed ornaments if any
      if (note->getNoteDelayedTurnOrnament ()) {
        // c2*2/3 ( s2*1/3\turn JMI
        fLilypondCodeStream <<
          "*" <<
          gGlobalLpsr2lilypondOahGroup->getDelayedOrnamentsFraction ();
      }

/* JMI
      // print the tie if any
      {
        S_msrTie noteTie = note->getNoteTie ();

        if (noteTie) {
          if (noteTie->getTieKind () == msrTie::kTieStart) {
            fLilypondCodeStream <<
              "%{line " << inputLineNumber << "%}" <<
              " ~ %{kDoubleTremoloMemberNote%}";
          }
        }
      }
*/

      // this note is the new relative octave reference
      switch (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) {
        case kOctaveEntryRelative:
          fCurrentOctaveEntryReference = note;
          break;
        case kOctaveEntryAbsolute:
          break;
        case kOctaveEntryFixed:
          break;
      } // switch
      break;

    case msrNote::kGraceNote:
      // print the note name
      fLilypondCodeStream <<
        notePitchAsLilypondString (note);

      // print the grace note's graphic duration
      fLilypondCodeStream <<
        msrDurationKindAsString (
          note->
            getNoteGraphicDurationKind ());

      // print the dots if any JMI ???
      for (int i = 0; i < note->getNoteDotsNumber (); i++) {
        fLilypondCodeStream << ".";
      } // for

      // don't print the tie if any, 'acciacattura takes care of it
      /*
      {
        S_msrTie noteTie = note->getNoteTie ();

        if (noteTie) {
          if (noteTie->getTieKind () == msrTie::kTieStart) {
            fLilypondCodeStream <<
              "%{line " << inputLineNumber << "%}" <<
              "~  %{kGraceNote%}";
          }
        }
      }
      */

      // this note is the new relative octave reference
      switch (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) {
        case kOctaveEntryRelative:
          fCurrentOctaveEntryReference = note;
          break;
        case kOctaveEntryAbsolute:
          break;
        case kOctaveEntryFixed:
          break;
      } // switch
      break;

    case msrNote::kGraceSkipNote:
      // print the note name
      if (gGlobalLpsr2lilypondOahGroup->getGeneratePositionsInMeasures ()) {
        // print the rest name to help pin-point bugs
        fLilypondCodeStream << "r%{333%}";
      }
      else {
        // print the skip name
        fLilypondCodeStream << "s%{444%}";
      }

      // print the skip duration
      fLilypondCodeStream <<
        durationAsLilypondString (
          inputLineNumber,
          note->
            getNoteDisplayWholeNotes ());

      // print the dots if any JMI ???
      for (int i = 0; i < note->getNoteDotsNumber (); i++) {
        fLilypondCodeStream << ".";
      } // for

      // a grace skip is no relative octave reference,
      // the preceding one is kept
      break;

    case msrNote::kGraceChordMemberNote:
      // print the note name
      fLilypondCodeStream <<
        notePitchAsLilypondString (note);

      // dont't print the grace note's graphic duration

      // print the dots if any JMI ???
      for (int i = 0; i < note->getNoteDotsNumber (); i++) {
        fLilypondCodeStream << ".";
      } // for

      // don't print the tie if any, 'acciacattura takes care of it
      /*
      {
        S_msrTie noteTie = note->getNoteTie ();

        if (noteTie) {
          if (noteTie->getTieKind () == msrTie::kTieStart) {
            fLilypondCodeStream <<
              "%{line " << inputLineNumber << "%}" <<
              "~  %{kGraceChordMemberNote%}";
          }
        }
      }
      */

      // inside chords, a note is relative to the preceding one
      switch (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) {
        case kOctaveEntryRelative:
          fCurrentOctaveEntryReference = note;
          break;
        case kOctaveEntryAbsolute:
          break;
        case kOctaveEntryFixed:
          break;
      } // switch
      break;

    case msrNote::kChordMemberNote:
      {
        // print the note name
        fLilypondCodeStream <<
          notePitchAsLilypondString (note);

        // don't print the note duration,
        // it will be printed for the chord itself

        // don't print the string number if any,
        // it should appear after the chord itself
        const list<S_msrTechnicalWithInteger>&
          chordMemberNoteTechnicalsWithIntegers =
            note->getNoteTechnicalWithIntegers ();

        if (chordMemberNoteTechnicalsWithIntegers.size ()) {
          list<S_msrTechnicalWithInteger>::const_iterator i;
          for (
            i=chordMemberNoteTechnicalsWithIntegers.begin ();
            i!=chordMemberNoteTechnicalsWithIntegers.end ();
            i++
          ) {
            S_msrTechnicalWithInteger
              technicalWithInteger = (*i);

            switch (technicalWithInteger->getTechnicalWithIntegerKind ()) {
              case msrTechnicalWithInteger::kFingering:
                break;
              case msrTechnicalWithInteger::kFret:
                break;
              case msrTechnicalWithInteger::kString:
                if (fOnGoingChord) {
                  fPendingChordMemberNotesStringNumbers.push_back (
                    technicalWithInteger->
                        getTechnicalWithIntegerValue ());
                }
                break;
            } // switch
          } // for
        }

        // inside chords, a note is relative to the preceding one
        switch (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) {
          case kOctaveEntryRelative:
            fCurrentOctaveEntryReference = note;
            break;
          case kOctaveEntryAbsolute:
            break;
          case kOctaveEntryFixed:
            break;
        } // switch
      }
      break;

    case msrNote::kTupletMemberNote:
      if (gGlobalLpsr2lilypondOahGroup->getIndentTuplets ()) {
        fLilypondCodeStream << endl;
      }

      // print the note name
      fLilypondCodeStream <<
        notePitchAsLilypondString (note);

      // print the note display duration
      fLilypondCodeStream <<
        durationAsLilypondString (
          inputLineNumber,
          note->
            getNoteDisplayWholeNotes ());

/* JMI
      // print the tie if any
      {
        S_msrTie noteTie = note->getNoteTie ();

        if (noteTie) {
          if (noteTie->getTieKind () == msrTie::kTieStart) {
            fLilypondCodeStream <<
              "%{line " << inputLineNumber << "%}" <<
              "~  %{kTupletMemberNote%}"; // JMI spaces???
          }
        }
      }
*/

      // this note is the new relative octave reference
      switch (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) {
        case kOctaveEntryRelative:
          fCurrentOctaveEntryReference = note;
          break;
        case kOctaveEntryAbsolute:
          break;
        case kOctaveEntryFixed:
          break;
      } // switch
      break;

    case msrNote::kTupletRestMemberNote:
      if (gGlobalLpsr2lilypondOahGroup->getIndentTuplets ()) {
        fLilypondCodeStream << endl;
      }

      // print the note name
      fLilypondCodeStream <<
        string (
          note->getNoteOccupiesAFullMeasure ()
            ? "s%{6%}" // JMI ??? "R"
            : "r%{5%}");

      // print the note display duration
      fLilypondCodeStream <<
        durationAsLilypondString (
          inputLineNumber,
          note->
            getNoteDisplayWholeNotes ());

/* JMI
      // print the tie if any
      {
        S_msrTie noteTie = note->getNoteTie ();

        if (noteTie) {
          if (noteTie->getTieKind () == msrTie::kTieStart) {
            fLilypondCodeStream <<
              "%{line " << inputLineNumber << "%}" <<
              "~  %{kTupletMemberNote%}"; // JMI spaces???
          }
        }
      }
*/

      // a rest is no relative octave reference,
      break;

    case msrNote::kTupletUnpitchedMemberNote:
      if (gGlobalLpsr2lilypondOahGroup->getIndentTuplets ()) {
        fLilypondCodeStream << endl;
      }

      // print the note name
      fLilypondCodeStream <<
        "e"; // by convention

      // print the note (display) duration
      fLilypondCodeStream <<
        durationAsLilypondString (
          inputLineNumber,
          note->
            getNoteDisplayWholeNotes ());

/* JMI
      // print the tie if any
      {
        S_msrTie noteTie = note->getNoteTie ();

        if (noteTie) {
          if (noteTie->getTieKind () == msrTie::kTieStart) {
            fLilypondCodeStream <<
              "%{line " << inputLineNumber << "%}" <<
              "~  %{kTupletUnpitchedMemberNote%}";
          }
        }
      }
      */
      break;

    case msrNote::kGraceTupletMemberNote:
      if (gGlobalLpsr2lilypondOahGroup->getIndentTuplets ()) {
        fLilypondCodeStream << endl;
      }

      // print the note name
      if (note->getNoteIsARest ()) {
        fLilypondCodeStream <<
          string (
            note->getNoteOccupiesAFullMeasure ()
              ? "R%{4%}"
              : "r%{6%}");
      }
      else {
        fLilypondCodeStream <<
          notePitchAsLilypondString (note);
      }

      // print the note display duration
      fLilypondCodeStream <<
        durationAsLilypondString (
          inputLineNumber,
          note->
            getNoteDisplayWholeNotes ());

      // print the tie if any
      {
        S_msrTie noteTie = note->getNoteTie ();

        if (noteTie) {
          if (noteTie->getTieKind () == msrTie::kTieStart) {
            fLilypondCodeStream <<
              "%{line " << inputLineNumber << "%}" <<
              "~  %{kGraceTupletMemberNote%}"; // JMI spaces???
          }
        }
      }

      // this note is no new relative octave reference JMI ???
      // this note is the new relative octave reference
      switch (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) {
        case kOctaveEntryRelative:
          fCurrentOctaveEntryReference = note;
          break;
        case kOctaveEntryAbsolute:
          break;
        case kOctaveEntryFixed:
          break;
      } // switch
      break;
  } // switch

  fLilypondCodeStream << ' ';
}

void lpsr2lilypondTranslator::generateCodeRightAfterNote (
  S_msrNote note)
{
  if (! fOnGoingChord) {
    // generate the note codas if any
    const list<S_msrCoda>&
      noteCodas =
        note->getNoteCodas ();

    if (noteCodas.size ()) {
      list<S_msrCoda>::const_iterator i;
      for (i=noteCodas.begin (); i!=noteCodas.end (); i++) {
        S_msrCoda coda = (*i);

        // generate only the first coda before the note
        switch (coda->getCodaKind ()) {
          case msrCoda::kCodaFirst:
            // generate the coda
            generateCoda (coda);
            break;
          case msrCoda::kCodaSecond:
            break;
        } // switch
      } // for
    }

    // generate the note dal segnos if any
    const list<S_msrDalSegno>&
      noteDalSegnos =
        note->getNoteDalSegnos ();

    if (noteDalSegnos.size ()) {
      list<S_msrDalSegno>::const_iterator i;
      for (i=noteDalSegnos.begin (); i!=noteDalSegnos.end (); i++) {
        // generate the dal segno
        fLilypondCodeStream <<
          endl <<
          "\\override Score.RehearsalMark.break-visibility = #begin-of-line-invisible" <<
          endl <<
          "\\tweak self-alignment-X #RIGHT" <<
          endl <<
          "\\mark \\markup { " <<
          (*i)->getDalSegnoString () <<
          " }" <<
          endl;
      } // for
    }
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generateNoteArticulation (
  S_msrArticulation articulation)
{
  // should the placement be generated?
  bool doGeneratePlacement = true;

  // generate note articulation preamble if any
  switch (articulation->getArticulationKind ()) {
    case msrArticulation::k_NoArticulation:
       break;

    case msrArticulation::kAccent:
       break;
    case msrArticulation::kBreathMark:
      doGeneratePlacement = false;
      break;
    case msrArticulation::kCaesura:
      doGeneratePlacement = false;
      break;
    case msrArticulation::kSpiccato:
      doGeneratePlacement = false;
      break;
    case msrArticulation::kStaccato:
      doGeneratePlacement = true;
      break;
    case msrArticulation::kStaccatissimo:
      doGeneratePlacement = true;
      break;
    case msrArticulation::kStress:
      doGeneratePlacement = false;
      break;
    case msrArticulation::kUnstress:
      doGeneratePlacement = false;
      break;
    case msrArticulation::kDetachedLegato:
      doGeneratePlacement = true;
      break;
    case msrArticulation::kStrongAccent:
      doGeneratePlacement = true;
      break;
    case msrArticulation::kTenuto:
      doGeneratePlacement = true;
      break;

    case msrArticulation::kFermata:
      doGeneratePlacement = true;
      break;

    case msrArticulation::kArpeggiato:
      // this is handled in chordArticulationAsLilyponString ()
      doGeneratePlacement = false;
      break;
    case msrArticulation::kNonArpeggiato:
      // this is handled in chordArticulationAsLilyponString ()
      doGeneratePlacement = false;
      break;

    case msrArticulation::kDoit:
      doGeneratePlacement = true;
      break;
    case msrArticulation::kFalloff:
      doGeneratePlacement = true;
      break;
    case msrArticulation::kPlop:
      doGeneratePlacement = false;
      break;
    case msrArticulation::kScoop:
      doGeneratePlacement = false;
      break;
  } // switch

  if (doGeneratePlacement) {
    switch (articulation->getArticulationPlacementKind ()) {
      case kPlacementNone:
        fLilypondCodeStream << "-";
        break;
      case kPlacementAbove:
        fLilypondCodeStream << "^";
        break;
      case kPlacementBelow:
        fLilypondCodeStream << "_";
        break;
    } // switch
  }

  // generate note articulation itself
  switch (articulation->getArticulationKind ()) {
    case msrArticulation::k_NoArticulation:
      fLilypondCodeStream << ">";
      break;

    case msrArticulation::kAccent:
      fLilypondCodeStream << ">";
      break;
    case msrArticulation::kBreathMark:
      fLilypondCodeStream << "\\breathe";
      break;
    case msrArticulation::kCaesura:
    /* JMI
          fLilypondCodeStream <<
            endl <<
            R"(\once\override BreathingSign.text = \markup {\musicglyph #"scripts.caesura.straight"} \breathe)" <<
            endl;
     */
      fLilypondCodeStream <<
        endl <<
        "\\override BreathingSign.text = \\markup {"
        "\\musicglyph #\"scripts.caesura.curved\"}" <<
        endl <<
        "\\breathe" <<
        endl;
      break;
    case msrArticulation::kSpiccato:
      fLilypondCodeStream <<
        "%{spiccato???%}";
      break;
    case msrArticulation::kStaccato:
      fLilypondCodeStream <<
        ".";
      break;
    case msrArticulation::kStaccatissimo:
      fLilypondCodeStream <<
        "!";
      break;
    case msrArticulation::kStress:
      fLilypondCodeStream <<
        "%{stress???%}";
      break;
    case msrArticulation::kUnstress:
      fLilypondCodeStream <<
        "%{unstress???%}";
      break;
    case msrArticulation::kDetachedLegato:
      fLilypondCodeStream <<
        "_"; // portato
      break;
    case msrArticulation::kStrongAccent:
      fLilypondCodeStream <<
        "^"; // marcato
      break;
    case msrArticulation::kTenuto:
      fLilypondCodeStream <<
        "-";
      break;

    case msrArticulation::kFermata:
      if (
        // fermata?
        S_msrFermata
          fermata =
            dynamic_cast<msrFermata*>(&(*articulation))
        ) {
        switch (fermata->getFermataTypeKind ()) {
          case msrFermata::kFermataTypeNone:
            // no placement needed
            break;
          case msrFermata::kFermataTypeUpright:
            // no placement needed
            break;
          case msrFermata::kFermataTypeInverted:
            fLilypondCodeStream << "_";
            break;
        } // switch

        switch (fermata->getFermataKind ()) {
          case msrFermata::kNormalFermataKind:
            fLilypondCodeStream << "\\fermata ";
            break;
          case msrFermata::kAngledFermataKind:
            fLilypondCodeStream << "\\shortfermata ";
            break;
          case msrFermata::kSquareFermataKind:
            fLilypondCodeStream << "\\longfermata ";
            break;
        } // switch
      }
      else {
        stringstream s;

        s <<
          "note articulation '" <<
          articulation->asString () <<
          "' has 'fermata' kind, but is not of type S_msrFermata" <<
          ", line " << articulation->getInputLineNumber ();

        msrInternalError (
          gGlobalOahOahGroup->getInputSourceName (),
          articulation->getInputLineNumber (),
          __FILE__, __LINE__,
          s.str ());
      }
      break;

    case msrArticulation::kArpeggiato:
      // this is handled in chordArticulationAsLilyponString ()
      break;
    case msrArticulation::kNonArpeggiato:
      // this is handled in chordArticulationAsLilyponString ()
      break;
    case msrArticulation::kDoit:
      fLilypondCodeStream <<
        "\\bendAfter #+4";
      break;
    case msrArticulation::kFalloff:
      fLilypondCodeStream <<
        "\\bendAfter #-4";
      break;
    case msrArticulation::kPlop:
      fLilypondCodeStream <<
        "%{plop???%}";
      break;
    case msrArticulation::kScoop:
      fLilypondCodeStream <<
        "%{scoop???%}";
      break;
  } // switch
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generateChordArticulation (
  S_msrArticulation articulation)
{
  switch (articulation->getArticulationPlacementKind ()) {
    case kPlacementNone:
      fLilypondCodeStream << "-";
      break;
    case kPlacementAbove:
      fLilypondCodeStream << "^";
      break;
    case kPlacementBelow:
      fLilypondCodeStream << "_";
      break;
  } // switch

  switch (articulation->getArticulationKind ()) {
    case msrArticulation::k_NoArticulation:
      break;

    case msrArticulation::kAccent:
      fLilypondCodeStream << ">";
      break;
    case msrArticulation::kBreathMark:
      fLilypondCodeStream << "\\breathe";
      break;
    case msrArticulation::kCaesura:
    /* JMI
          fLilypondCodeStream <<
            endl <<
            R"(\once\override BreathingSign.text = \markup {\musicglyph #"scripts.caesura.straight"} \breathe)" <<
            endl;
     */
      fLilypondCodeStream <<
        endl <<
        "\\override BreathingSign.text = \\markup {"
        "\\musicglyph #\"scripts.caesura.curved\"}" <<
        endl <<
      "\\breathe" <<
        endl;
      break;
    case msrArticulation::kSpiccato:
      fLilypondCodeStream <<
        "%{spiccato???%}";
      break;
    case msrArticulation::kStaccato:
      fLilypondCodeStream <<
        "\\staccato"; // JMI "-.";
      break;
    case msrArticulation::kStaccatissimo:
      fLilypondCodeStream << "!";
      break;
    case msrArticulation::kStress:
      fLilypondCodeStream <<
        "%{stress???%}";
      break;
    case msrArticulation::kUnstress:
      fLilypondCodeStream <<
        "%{unstress%}";
      break;
    case msrArticulation::kDetachedLegato:
      fLilypondCodeStream <<
        "_"; // portato
      break;
    case msrArticulation::kStrongAccent:
      fLilypondCodeStream <<
        "^"; // marcato
      break;
    case msrArticulation::kTenuto:
      fLilypondCodeStream << "-";
      break;

    case msrArticulation::kFermata:
      if (
        // fermata?
        S_msrFermata
          fermata =
            dynamic_cast<msrFermata*>(&(*articulation))
        ) {
        switch (fermata->getFermataTypeKind ()) {
          case msrFermata::kFermataTypeNone:
            // no placement needed
            break;
          case msrFermata::kFermataTypeUpright:
            // no placement needed
            break;
          case msrFermata::kFermataTypeInverted:
            fLilypondCodeStream << "_";
            break;
        } // switch

        switch (fermata->getFermataKind ()) {
          case msrFermata::kNormalFermataKind:
            fLilypondCodeStream << "\\fermata ";
            break;
          case msrFermata::kAngledFermataKind:
            fLilypondCodeStream << "\\shortfermata ";
            break;
          case msrFermata::kSquareFermataKind:
            fLilypondCodeStream << "\\longfermata ";
            break;
        } // switch
      }
      else {
        stringstream s;

        s <<
          "chord articulation '" <<
          articulation->asString () <<
          "' has 'fermata' kind, but is not of type S_msrFermata" <<
          ", line " << articulation->getInputLineNumber ();

        msrInternalError (
          gGlobalOahOahGroup->getInputSourceName (),
          articulation->getInputLineNumber (),
          __FILE__, __LINE__,
          s.str ());
      }
      break;

    case msrArticulation::kArpeggiato:
      fLilypondCodeStream <<
        "\\arpeggio";
      break;
    case msrArticulation::kNonArpeggiato:
      fLilypondCodeStream <<
        "\\arpeggio";
      break;
    case msrArticulation::kDoit:
      fLilypondCodeStream <<
        "\\bendAfter #+4";
      break;
    case msrArticulation::kFalloff:
      fLilypondCodeStream <<
        "\\bendAfter #-4";
      break;
    case msrArticulation::kPlop:
      fLilypondCodeStream <<
        "%{plop%}";
      break;
    case msrArticulation::kScoop:
      fLilypondCodeStream <<
        "%{scoop%}";
      break;
  } // switch
}

//________________________________________________________________________
string lpsr2lilypondTranslator::technicalAsLilypondString (
  S_msrTechnical technical)
{
  string result;

  switch (technical->getTechnicalKind ()) {
    case msrTechnical::kArrow:
      result = "%{\\Arrow???%}";
      break;
    case msrTechnical::kDoubleTongue:
      result = "-\\tongue #2";
      break;
    case msrTechnical::kDownBow:
      result = "\\downbow";
      break;
    case msrTechnical::kFingernails:
      result = "%{\\Fingernails???%}";
      break;
    case msrTechnical::kHarmonic:
      result = "\\flageolet"; // JMI "\\once\\override Staff.NoteHead.style = #'harmonic-mixed";
      break;
    case msrTechnical::kHeel:
      result = "\\lheel"; // rheel??? JMI
      break;
    case msrTechnical::kHole:
      result = "%{\\Hole???%}";
      break;
    case msrTechnical::kOpenString:
      result = "\\open"; // halfopen ??? JMI
      break;
    case msrTechnical::kSnapPizzicato:
      result = "\\snappizzicato";
      break;
    case msrTechnical::kStopped:
      result = "\\stopped"; // or -+ JMI
      break;
    case msrTechnical::kTap:
      result = "%{\\Tap???%}";
      break;
    case msrTechnical::kThumbPosition:
      result = "\\thumb";
      break;
    case msrTechnical::kToe:
      result = "\\ltoe"; // rtoe ??? JMI
      break;
    case msrTechnical::kTripleTongue:
      result = "-\\tongue #3";
      break;
    case msrTechnical::kUpBow:
      result = "\\upbow";
      break;
  } // switch

  return result;
}

//________________________________________________________________________
string lpsr2lilypondTranslator::technicalWithIntegerAsLilypondString (
  S_msrTechnicalWithInteger technicalWithInteger)
{
  stringstream s;

  switch (technicalWithInteger->getTechnicalWithIntegerKind ()) {
     case msrTechnicalWithInteger::kFingering:
      s <<
        "- " <<
       technicalWithInteger->
          getTechnicalWithIntegerValue ();
      break;
    case msrTechnicalWithInteger::kFret:
      // LilyPond will take care of that JMI
      break;
    case msrTechnicalWithInteger::kString:
      s << // no space is allowed between the backSlash and the number
        "\\" <<
       technicalWithInteger->
          getTechnicalWithIntegerValue ();
      break;
  } // switch

  return s.str ();
}

//________________________________________________________________________
string lpsr2lilypondTranslator::technicalWithFloatAsLilypondString (
  S_msrTechnicalWithFloat technicalWithFloat)
{
  stringstream s;

  switch (technicalWithFloat->getTechnicalWithFloatKind ()) {
    case msrTechnicalWithFloat::kBend:
      s <<
        "\\bendAfter " <<
       technicalWithFloat->
          getTechnicalWithFloatValue ();
      break;
  } // switch

  return s.str ();
}

//________________________________________________________________________
string lpsr2lilypondTranslator::technicalWithStringAsLilypondString (
  S_msrTechnicalWithString technicalWithString)
{
  string result;

  switch (technicalWithString->getTechnicalWithStringKind ()) {
    case msrTechnicalWithString::kHammerOn:
      break;
    case msrTechnicalWithString::kHandbell:
      result = "%{\\Handbell???%}";
      break;
    case msrTechnicalWithString::kOtherTechnical:
      result = "%{\\OtherTechnical???%}";
      break;
    case msrTechnicalWithString::kPluck:
      result = "%{Pluck???%}";
      break;
    case msrTechnicalWithString::kPullOff:
      break;
  } // switch

  string stringValue =
    technicalWithString->
      getTechnicalWithStringValue ();

  if (stringValue.size ()) {
    result +=
      string (" ") +
      "-\\markup {\"" + stringValue + "\"}";
  }

  return result;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generateOrnament (
  S_msrOrnament ornament)
{
  S_msrNote
    ornamentNoteUpLink =
      ornament->
        getOrnamentNoteUpLink ();

  string
    noteUpLinkDuration =
      ornamentNoteUpLink->
        noteSoundingWholeNotesAsMsrString ();

  switch (ornament->getOrnamentKind ()) {
    case msrOrnament::kOrnamentTrill:
      if (! ornamentNoteUpLink->getNoteWavyLineSpannerStart ()) {
        fLilypondCodeStream <<
          "\\trill ";
      }
      else {
        fLilypondCodeStream <<
          "\\startTrillSpan ";
      }
      break;

/* JMI
    case msrOrnament::kOrnamentDashes:
      if (! ornamentNoteUpLink->getNoteWavyLineSpannerStart ()) {
        fLilypondCodeStream <<
          "%{\\dashes%} ";
      }
      break;
*/

    case msrOrnament::kOrnamentTurn:
      fLilypondCodeStream <<
          "\\turn ";
      break;

    case msrOrnament::kOrnamentInvertedTurn:
      fLilypondCodeStream <<
          "\\reverseturn ";
      break;

    case msrOrnament::kOrnamentDelayedTurn:
      {
        // c2*2/3  s2*1/3\turn
        rational
          remainingFraction =
            rational (1, 1)
              -
            gGlobalLpsr2lilypondOahGroup->getDelayedOrnamentsFraction ();

        int
          numerator =
            remainingFraction.getNumerator (),
          denominator =
            remainingFraction.getDenominator ();

        fLilypondCodeStream <<
          "s%{16%}" <<
          noteUpLinkDuration <<
          "*" <<
            denominator
            -
            numerator <<
          "/" <<
            denominator <<
          "\\turn ";

        // forget about the last found whole notes duration,
        // since the latter has been multipled by fDelayedOrnamentsFraction
        fLastMetWholeNotes = rational (0, 1);
      }
      break;

    case msrOrnament::kOrnamentDelayedInvertedTurn:
      {
/* JMI
        fLilypondCodeStream <<
          "delayed inverted turn is not supported, replaced by inverted turn," <<
          endl <<
          "see http://lilypond.org/doc/v2.18/Documentation/snippets/expressive-marks";

        lpsrMusicXMLWarning (
          inputLineNumber,
          s.str ());

        result = "\\reverseturn %{ " + s.str () + " %}";
*/
        // c2*2/3 ( s2*1/3\turn

        fLilypondCodeStream <<
          "s%{7%}" <<
          noteUpLinkDuration <<
          "*1/3" "\\reverseturn ";
      }
      break;

    case msrOrnament::kOrnamentVerticalTurn:
      fLilypondCodeStream <<
        "^\\markup { \\rotate #90 \\musicglyph #\"scripts.turn\" } ";
          /* JMI
      {
        string message =
          "delayed vertical turn is not supported, ignored";

        lpsrMusicXMLWarning (
          inputLineNumber,
          message);

        result = "%{ " + message + " %}";
      }
        */
      break;

    case msrOrnament::kOrnamentMordent:
      fLilypondCodeStream <<
        "\\mordent ";
      break;

    case msrOrnament::kOrnamentInvertedMordent:
      fLilypondCodeStream <<
        "\\prall ";
      break;
      \
    case msrOrnament::kOrnamentSchleifer:
      fLilypondCodeStream <<
        "%{\\schleifer???%} ";
      break;

    case msrOrnament::kOrnamentShake:
      fLilypondCodeStream <<
        "%{\\shake???%} ";
      break;

    case msrOrnament::kOrnamentAccidentalKind:
      switch (ornament->getOrnamentPlacementKind ()) {
        case kPlacementNone:
          fLilypondCodeStream << "-";
          break;
        case kPlacementAbove:
          fLilypondCodeStream << "^";
          break;
        case kPlacementBelow:
          fLilypondCodeStream << "_";
          break;
      } // switch

string result; // JMI
      switch (ornament->getOrnamentAccidentalKind ()) {
        case kAccidentalNone:
          fLilypondCodeStream << "\\markup { \\accidentalNone } ";
          break;

        case kAccidentalSharp:
          fLilypondCodeStream << "\\markup { \\aSharp } ";
          break;
        case kAccidentalNatural:
          fLilypondCodeStream << "\\markup { \\aNatural } ";
          break;
        case kAccidentalFlat:
          fLilypondCodeStream << "\\markup { \\aFlat } ";
          break;
        case kAccidentalDoubleSharp:
          fLilypondCodeStream << "\\markup { \\doublesharp } ";
          fLilypondCodeStream << "\\markup { \\doubleflat } ";
          break;
        case kAccidentalSharpSharp:
          fLilypondCodeStream << "\\markup { \\aSharpSharp } ";
          break;
        case kAccidentalFlatFlat:
          fLilypondCodeStream << "\\markup { \\aFlatFlat } ";
          break;
        case kAccidentalNaturalSharp:
          fLilypondCodeStream << "\\markup { \\aNaturalSharp } ";
          break;
        case kAccidentalNaturalFlat:
          fLilypondCodeStream << "\\markup { \\aNaturalFlat } ";
          break;
        case kAccidentalQuarterFlat:
          fLilypondCodeStream << "\\markup { \\aQuarterFlat } ";
          break;
        case kAccidentalQuarterSharp:
          fLilypondCodeStream << "\\markup { \\aQuarterSharp } ";
          break;
        case kAccidentalThreeQuartersFlat:
          fLilypondCodeStream << "\\markup { \\aThreeQuartersFlat } ";
          break;
        case kAccidentalThreeQuartersSharp:
          fLilypondCodeStream << "\\markup { \\aThreeQuartersSharp } ";
          break;

        case kAccidentalSharpDown:
          fLilypondCodeStream << "\\markup { \\aSharpDown } ";
          break;
        case kAccidentalSharpUp:
          fLilypondCodeStream << "\\markup { \\aSharpUp } ";
          break;
        case kAccidentalNaturalDown:
          fLilypondCodeStream << "\\markup { \\aNaturalDown } ";
          break;
        case kAccidentalNaturalUp:
          fLilypondCodeStream << "\\markup { \\aNaturalUp } ";
          break;
        case kAccidentalFlatDown:
          fLilypondCodeStream << "\\markup { \\aFlatDown } ";
          break;
        case kAccidentalFlatUp:
          fLilypondCodeStream << "\\markup { \\aFlatUp } ";
          break;
        case kAccidentalTripleSharp:
          fLilypondCodeStream << "\\markup { \\aTripleSharp } ";
          break;
        case kAccidentalTripleFlat:
          fLilypondCodeStream << "\\markup { \\aTripleFlat } ";
          break;
        case kAccidentalSlashQuarterSharp:
          fLilypondCodeStream << "\\markup { \\aSlashQuarterSharp } ";
          break;
        case kAccidentalSlashSharp:
          fLilypondCodeStream << "\\markup { \\aSlashSharp } ";
          break;
        case kAccidentalSlashFlat:
          fLilypondCodeStream << "\\markup { \\aSlashFlat } ";
          break;
        case kAccidentalDoubleSlashFlat:
          fLilypondCodeStream << "\\markup { \\adoubleSlashFlat } ";
          break;
        case kAccidentalSharp_1:
          fLilypondCodeStream << "\\markup { \\aSharp_1 } ";
          break;
        case kAccidentalSharp_2:
          fLilypondCodeStream << "\\markup { \\aSharp_2 } ";
          break;
        case kAccidentalSharp_3:
          fLilypondCodeStream << "\\markup { \\aSharp_3 } ";
          break;
        case kAccidentalSharp_5:
          fLilypondCodeStream << "\\markup { \\aSharp_5 } ";
          break;
        case kAccidentalFlat_1:
          fLilypondCodeStream << "\\markup { \\aFlat_1 } ";
          break;
        case kAccidentalFlat_2:
          fLilypondCodeStream << "\\markup { \\aFlat_2 } ";
          break;
        case kAccidentalFlat_3:
          fLilypondCodeStream << "\\markup { \\aFlat_3 } ";
          break;
        case kAccidentalFlat_4:
          fLilypondCodeStream << "\\markup { \\aFlat_4 } ";
          break;
        case kAccidentalSori:
          fLilypondCodeStream << "\\markup { \\aSori } ";
          break;
        case kAccidentalKoron:
          fLilypondCodeStream << "\\markup { \\aKoron } ";
          break;

        case kAccidentalOther:
          fLilypondCodeStream << "\\markup { \\aOther } ";
          break;
      } // switch
      break;
  } // switch
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generateCodeForSpannerBeforeNote (
  S_msrSpanner spanner)
{
  switch (spanner->getSpannerKind ()) {

    case msrSpanner::kSpannerDashes:
      switch (spanner->getSpannerTypeKind ()) {
        case kSpannerTypeStart:
          fLilypondCodeStream <<
            "\\once \\override TextSpanner.style = #'dashed-line" <<
            endl;
          fOnGoingTrillSpanner = true;
          break;
        case kSpannerTypeStop:
//          fLilypondCodeStream <<
//            "\\stopTextSpan ";
          break;
        case kSpannerTypeContinue:
          break;
        case k_NoSpannerType:
          break;
      } // switch
      break;

    case msrSpanner::kSpannerWavyLine:
      switch (spanner->getSpannerTypeKind ()) {
        case kSpannerTypeStart:
          if (spanner->getSpannerNoteUpLink ()->getNoteTrillOrnament ()) {
            // don't generate anything, the trill will display the wavy line
            fOnGoingTrillSpanner = true;
          }
          else {
            fLilypondCodeStream <<
              "\\once \\override TextSpanner.style = #'trill" <<
              endl;
          }
          break;
        case kSpannerTypeStop:
          break;
        case kSpannerTypeContinue:
          break;
        case k_NoSpannerType:
          break;
      } // switch

      msrPlacementKind
        spannerPlacementKind =
          spanner->getSpannerPlacementKind ();

      if (spannerPlacementKind != fCurrentSpannerPlacementKind) {
        switch (spannerPlacementKind) {
          case msrPlacementKind::kPlacementNone:
            break;
          case msrPlacementKind::kPlacementAbove:
            fLilypondCodeStream <<
              "\\textSpannerUp ";
            break;
          case msrPlacementKind::kPlacementBelow:
            fLilypondCodeStream <<
              "\\textSpannerDown ";
            break;
          } // switch

        fCurrentSpannerPlacementKind = spannerPlacementKind;
      }
      break;
  } // switch
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generateCodeForSpannerAfterNote (
  S_msrSpanner spanner)
{
  switch (spanner->getSpannerKind ()) {

    case msrSpanner::kSpannerDashes:
      switch (spanner->getSpannerTypeKind ()) {
        case kSpannerTypeStart:
          fLilypondCodeStream <<
            "\\startTextSpan ";
          fOnGoingTrillSpanner = true;
          break;
        case kSpannerTypeStop:
          fLilypondCodeStream <<
            "\\stopTextSpan ";
          fOnGoingTrillSpanner = false;
          break;
        case kSpannerTypeContinue:
          break;
        case k_NoSpannerType:
          break;
      } // switch
      break;

    case msrSpanner::kSpannerWavyLine:
      switch (spanner->getSpannerTypeKind ()) {
        case kSpannerTypeStart:
          if (spanner->getSpannerNoteUpLink ()->getNoteTrillOrnament ()) {
            // don't generate anything, the trill will display the wavy line
            fOnGoingTrillSpanner = true;
          }
          else {
            fLilypondCodeStream <<
              "\\startTextSpan " <<
              endl;
          }
          break;
        case kSpannerTypeStop:
          {
            // get spanner start end
            S_msrSpanner
              spannerStartEnd =
                spanner->
                  getSpannerOtherEndSideLink ();

            // sanity check
            msgAssert (
              spannerStartEnd != nullptr,
              "spannerStartEnd is null");

            // has the start end a trill ornament?
            if (spannerStartEnd->getSpannerNoteUpLink ()->getNoteTrillOrnament ()) {
              fLilypondCodeStream <<
                "\\stopTrillSpan ";
            }
            else {
              fLilypondCodeStream <<
                "\\stopTextSpan ";
            }
            fOnGoingTrillSpanner = false;
          }
          break;
        case kSpannerTypeContinue:
          break;
        case k_NoSpannerType:
          break;
      } // switch

/* JMI
      msrPlacementKind
        spannerPlacementKind =
          spanner->getSpannerPlacementKind ();

      if (spannerPlacementKind != fCurrentSpannerPlacementKind) {
        switch (spannerPlacementKind) {
          case msrPlacementKind::kPlacementNone:
            break;
          case msrPlacementKind::kPlacementAbove:
            fLilypondCodeStream <<
              "\\textSpannerUp ";
            break;
          case msrPlacementKind::kPlacementBelow:
            fLilypondCodeStream <<
              "\\textSpannerDown ";
            break;
          } // switch

        fCurrentSpannerPlacementKind = spannerPlacementKind;
      }
      */

      break;
  } // switch
}

//________________________________________________________________________
string lpsr2lilypondTranslator::dynamicsAsLilypondString (
  S_msrDynamics dynamics)
{
  string result =
    "\\" + dynamics->dynamicsKindAsString ();

  return result;
}

//________________________________________________________________________
string lpsr2lilypondTranslator::harpPedalTuningAsLilypondString (
  msrAlterationKind alterationKind)
{
  string result;

  switch (alterationKind) {
    case kTripleFlat:
      result = "%{ tripleFlat %} ";
      break;
    case kDoubleFlat:
      result = "%{ doubleFlat %} ";
      break;
    case kSesquiFlat:
      result = "%{ sesquiFlat %} ";
      break;
    case kFlat:
      result = "^";
      break;
    case kSemiFlat:
      result = "?";
      break;
    case kNatural:
      result = "-";
      break;
    case kSemiSharp:
      result = "?";
      break;
    case kSharp:
      result = "v";
      break;
    case kSesquiSharp:
      result = "%{ sesquiSharp %} ";
      break;
    case kDoubleSharp:
      result = "%{ doubleSharp %} ";
      break;
    case kTripleSharp:
      result = "%{ tripleSharp %} ";
      break;
    case k_NoAlteration:
      result = "alteration???";
      break;
  } // switch

  return result;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::transposeDiatonicError (
  int inputLineNumber,
  int transposeDiatonic,
  int transposeChromatic)
{
  stringstream s;

  s <<
    "diatonic '" << transposeDiatonic <<
    "' is not consistent with " <<
    "chromatic '" << transposeChromatic <<
    "'";

  msrMusicXMLError (
    gGlobalOahOahGroup->getInputSourceName (),
    inputLineNumber,
    __FILE__, __LINE__,
    s.str ());
}

//________________________________________________________________________
string lpsr2lilypondTranslator::singleTremoloDurationAsLilypondString (
  S_msrSingleTremolo singleTremolo)
{
  int
    singleTremoloMarksNumber =
      singleTremolo->
        getSingleTremoloMarksNumber ();

/* JMI
  S_msrNote
    singleTremoloNoteUpLink =
      singleTremolo->
        getSingleTremoloNoteUpLink ();
*/

  msrDurationKind
    singleTremoloNoteDurationKind =
      singleTremolo->
        getSingleTremoloGraphicDurationKind ();
    /*
      singleTremoloNoteUpLink->
        getNoteGraphicDurationKind ();
    */

  /*
  The same output can be obtained by adding :N after the note,
  where N indicates the duration of the subdivision (it must be at least 8).
  If N is 8, one beam is added to the note’s stem.
  */

  int durationToUse =
    singleTremoloMarksNumber; // JMI / singleTremoloNoteSoundingWholeNotes;

  if (singleTremoloNoteDurationKind >= kEighth) {
    durationToUse +=
      1 + (singleTremoloNoteDurationKind - kEighth);
  }

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceTremolos ()) {
    gLogStream <<
      "singleTremoloDurationAsLilypondString()" <<
      ", line " << singleTremolo->getInputLineNumber () <<
      ", " <<
      singularOrPlural (
        singleTremoloMarksNumber, "mark", "marks") <<
      ", singleTremoloNoteDurationKind : " <<
      singleTremoloNoteDurationKind <<
      ", durationToUse : " <<
      durationToUse <<
      endl;
  }
#endif

  stringstream s;

  s <<
    ":" <<
 // JMI   int (pow (2, durationToUse + 2)) <<
      int (1 << (durationToUse + 2)) <<
    ' ';

  return s.str ();
}

//________________________________________________________________________
string lpsr2lilypondTranslator::harmonyDegreeAlterationKindAsLilypondString (
  msrAlterationKind harmonyDegreeAlterationKind)
{
  string result;

  switch (harmonyDegreeAlterationKind) {
    case k_NoAlteration:
      result = "?";
      break;
    case kTripleFlat:
      result = "?";
      break;
    case kDoubleFlat:
      result = "?";
      break;
    case kSesquiFlat:
      result = "?";
      break;
    case kFlat:
      result = "-";
      break;
    case kSemiFlat:
      result = "?";
      break;
    case kNatural:
      result = "";
      break;
    case kSemiSharp:
      result = "?";
      break;
    case kSharp:
      result = "+";
      break;
    case kSesquiSharp:
      result = "?";
      break;
    case kDoubleSharp:
      result = "?";
      break;
    case kTripleSharp:
      result = "?";
      break;
  } // switch

  return result;
}

string lpsr2lilypondTranslator::harmonyAsLilypondString (
  S_msrHarmony harmony)
{
  int inputLineNumber =
    harmony->getInputLineNumber ();

  stringstream s;

  // should '\powerChords' be generated?
  switch (harmony->getHarmonyKind ()) {
    case kPowerHarmony:
      if (! fPowerChordHaveAlreadyBeenGenerated) {
        s << "\\powerChords ";
        fPowerChordHaveAlreadyBeenGenerated = true;
      }
      break;
    default:
      ;
  } // switch

  // print harmony pitch
  s <<
    msrQuarterTonesPitchKindAsString (
      gGlobalMsrOahGroup->
        getMsrQuarterTonesPitchesLanguageKind (),
      harmony->
        getHarmonyRootQuarterTonesPitchKind ());

  // print harmony duration
  msrTupletFactor
    harmonyTupletFactor =
      harmony->getHarmonyTupletFactor ();

  if (harmonyTupletFactor.isEqualToOne ()) {
    // use harmony sounding whole notes
    s <<
      durationAsLilypondString (
        inputLineNumber,
        harmony->
          getHarmonySoundingWholeNotes ());
  }
  else {
    // use harmony display whole notes and tuplet factor
    s <<
      durationAsLilypondString (
        inputLineNumber,
        harmony->
          getHarmonyDisplayWholeNotes ()) <<
      "*" <<
      rational (1, 1) / harmonyTupletFactor.asRational ();
  }

  // print harmony kind
  switch (harmony->getHarmonyKind ()) {
    case k_NoHarmony:
      s << "Harmony???";
      break;

    // MusicXML harmonies

    case kMajorHarmony:
      s << ":5.3";
      break;
    case kMinorHarmony:
      s << ":m";
      break;
    case kAugmentedHarmony:
      s << ":aug";
      break;
    case kDiminishedHarmony:
      s << ":dim";
      break;

    case kDominantHarmony:
      s << ":7";
      break;
    case kMajorSeventhHarmony:
      s << ":maj7";
      break;
    case kMinorSeventhHarmony:
      s << ":m7";
      break;
    case kDiminishedSeventhHarmony:
      s << ":dim7";
      break;
    case kAugmentedSeventhHarmony:
      s << ":aug7";
      break;
    case kHalfDiminishedHarmony:
      s << ":m7.5-";
      break;
    case kMinorMajorSeventhHarmony:
      s << ":m7+";
      break;

    case kMajorSixthHarmony:
      s << ":6";
      break;
    case kMinorSixthHarmony:
      s << ":m6";
      break;

    case kDominantNinthHarmony:
      s << ":9";
      break;
    case kMajorNinthHarmony:
      s << ":maj7.9";
      break;
    case kMinorNinthHarmony:
      s << ":m7.9";
      break;

    case kDominantEleventhHarmony:
      s << ":11";
      break;
    case kMajorEleventhHarmony:
      s << ":maj7.11";
      break;
    case kMinorEleventhHarmony:
      s << ":m7.11";
      break;

    case kDominantThirteenthHarmony:
      s << ":13";
      break;
    case kMajorThirteenthHarmony:
      s << ":maj7.13";
      break;
    case kMinorThirteenthHarmony:
      s << ":m7.13";
      break;

    case kSuspendedSecondHarmony:
      s << ":sus2";
      break;
    case kSuspendedFourthHarmony:
      s << ":sus4";
      break;

/*
 * kNeapolitan f aes des' in:
 *
 * c e g c' -> f f aes des' -> d g d b -> c e g c'

they are three different pre-dominant chords that are taught to American undergrads in a sophomore theory course.

in E major:
Italian = C E A#
French = C E F# A#
German = C E G A#
Tristan = C D# F# A#

in all of them, the C and A# in theory want to fan out to B (the dominant).  This is, of course, in theory - Wagner’s use of the Tristan chord, which he clearly named his opera after, has the A# moving down to A, or the 7th of the dominant (I’m transposing to fit w/ the example above).  Wagner obviously did not pay much attention during his sophomore music theory course…
*/
    case kNeapolitanHarmony:
      s << ":3-.6-%{:Neapolitan%}";
      break;
    case kItalianHarmony:
      s << ":3.6+%{:Italian%}";
      break;
    case kFrenchHarmony:
      s << ":3.5+.6+%{:French%}";
      break;
    case kGermanHarmony:
      s << ":3.5.6+%{:German%}";
      break;

    case kPedalHarmony:
      s << "%{:Pedal%}";
      break;
    case kPowerHarmony:
      s << ":5 %{power%}";
      break;
    case kTristanHarmony:
      s << ":2+.5+.6+%{:Tristan%}";
      break;

    // jazz-specific chords

    case kMinorMajorNinth: // -maj9, minmaj9
      s << ":m9";
      break;

    case kDominantSuspendedFourthHarmony: // 7sus4, domsus4
      s << ":7sus4";
      break;
    case kDominantAugmentedFifthHarmony: // 7#5, domaug5
      s << ":7.5+";
      break;
    case kDominantMinorNinthHarmony: // 7b9, dommin9
      s << ":7.9-";
      break;
    case kDominantAugmentedNinthDiminishedFifthHarmony: // 7#9b5, domaug9dim5
      s << ":7.9+.5-";
      break;
    case kDominantAugmentedNinthAugmentedFifthHarmony: // 7#9#5, domaug9aug5
      s << ":7.9+.5+";
      break;
    case kDominantAugmentedEleventhHarmony: // 7#11, domaug11
      s << ":7.11+";
      break;

    case kMajorSeventhAugmentedEleventhHarmony: // maj7#11, maj7aug11
      s << ":maj7.11+";
      break;

    // other

    case kOtherHarmony:
      s << "%{:Other%}";
      break;

    case kNoneHarmony:
      s << "%{:None%}";
      break;
  } // switch

  // print harmony degrees if any
  list<S_msrHarmonyDegree>
    harmonyDegreesList =
      harmony->getHarmonyDegreesList ();

  if (harmonyDegreesList.size ()) {
    bool thereAreDegreesToBeRemoved = false;

    // print degrees to be added if any first
    for (
      list<S_msrHarmonyDegree>::const_iterator i = harmonyDegreesList.begin ();
      i != harmonyDegreesList.end ();
      i++
    ) {
      S_msrHarmonyDegree harmonyDegree = (*i);

      // get harmony degree information
      int
        harmonyDegreeValue =
          harmonyDegree->getHarmonyDegreeValue ();

      msrAlterationKind
        harmonyDegreeAlterationKind =
          harmonyDegree->
            getHarmonyDegreeAlterationKind ();

      msrHarmonyDegree::msrHarmonyDegreeTypeKind
        harmonyDegreeTypeKind =
          harmonyDegree->
            getHarmonyDegreeTypeKind ();

      // print the harmony degree
      switch (harmonyDegreeTypeKind) {
        case msrHarmonyDegree::kHarmonyDegreeTypeAdd:
          s <<
      // JMI ???      "." <<
            harmonyDegreeValue <<
            harmonyDegreeAlterationKindAsLilypondString (
              harmonyDegreeAlterationKind);
          break;

        case msrHarmonyDegree::kHarmonyDegreeTypeAlter:
          s <<
            "." <<
            harmonyDegreeValue <<
            harmonyDegreeAlterationKindAsLilypondString (
              harmonyDegreeAlterationKind);
          break;

        case msrHarmonyDegree::kHarmonyDegreeTypeSubstract:
          thereAreDegreesToBeRemoved = true;
          break;
      } // switch
    } // for

    // then print harmony degrees to be removed if any
    if (thereAreDegreesToBeRemoved) {
      s << "^";

      int counter = 0;

      for (
        list<S_msrHarmonyDegree>::const_iterator i = harmonyDegreesList.begin ();
        i != harmonyDegreesList.end ();
        i++
      ) {
        counter++;

        S_msrHarmonyDegree
          harmonyDegree = (*i);

        // get harmony degree information
        int
          harmonyDegreeValue =
            harmonyDegree->getHarmonyDegreeValue ();

        msrAlterationKind
          harmonyDegreeAlterationKind =
            harmonyDegree->
              getHarmonyDegreeAlterationKind ();

        msrHarmonyDegree::msrHarmonyDegreeTypeKind
          harmonyDegreeTypeKind =
            harmonyDegree->
              getHarmonyDegreeTypeKind ();

        // print the harmony degree
        switch (harmonyDegreeTypeKind) {
          case msrHarmonyDegree::kHarmonyDegreeTypeAdd:
          case msrHarmonyDegree::kHarmonyDegreeTypeAlter:
            break;

          case msrHarmonyDegree::kHarmonyDegreeTypeSubstract:
   // JMI         if (counter > 1) {}
          s <<
            harmonyDegreeValue <<
            harmonyDegreeAlterationKindAsLilypondString (
              harmonyDegreeAlterationKind);
            break;
        } // switch
      } // for
    }
  }

  // print the harmony bass if relevant
  msrQuarterTonesPitchKind
    harmonyBassQuarterTonesPitchKind =
      harmony->
        getHarmonyBassQuarterTonesPitchKind ();

  if (harmonyBassQuarterTonesPitchKind != k_NoQuarterTonesPitch_QTP) {
    s <<
      "/" <<
      msrQuarterTonesPitchKindAsString (
        gGlobalMsrOahGroup->
          getMsrQuarterTonesPitchesLanguageKind (),
        harmonyBassQuarterTonesPitchKind);
  }

  // print the harmony inversion if relevant // JMI ???
  int harmonyInversion =
    harmony->getHarmonyInversion ();

  if ( harmonyInversion!= K_HARMONY_NO_INVERSION) {
    s <<
      "%{ inversion: " << harmonyInversion << " %}";
  }

  return s.str ();
}

string lpsr2lilypondTranslator::figureAsLilypondString (
  S_msrFigure figure)
{
  stringstream s;

  fCurrentFiguredBassFiguresCounter++;

  // is the figured bass parenthesized?
  msrFiguredBass::msrFiguredBassParenthesesKind
    figuredBassParenthesesKind =
      fCurrentFiguredBass->
        getFiguredBassParenthesesKind ();

  // generate the figure number
  switch (figuredBassParenthesesKind) {
    case msrFiguredBass::kFiguredBassParenthesesYes:
      s << "[";
      break;
    case msrFiguredBass::kFiguredBassParenthesesNo:
      break;
  } // switch

  s <<
    figure->getFigureNumber ();

  switch (figuredBassParenthesesKind) {
    case msrFiguredBass::kFiguredBassParenthesesYes:
      s << "]";
      break;
    case msrFiguredBass::kFiguredBassParenthesesNo:
      break;
  } // switch

  // handle the figure prefix
  switch (figure->getFigurePrefixKind ()) {
    case msrFigure::k_NoFigurePrefix:
      break;
    case msrFigure::kDoubleFlatPrefix:
      s << "--";
      break;
    case msrFigure::kFlatPrefix:
      s << "-";
      break;
    case msrFigure::kFlatFlatPrefix:
      s << "flat flat";
      break;
    case msrFigure::kNaturalPrefix:
      s << "!";
      break;
    case msrFigure::kSharpSharpPrefix:
      s << "sharp sharp";
      break;
    case msrFigure::kSharpPrefix:
      s << "+";
      break;
    case msrFigure::kDoubleSharpPrefix:
      s << "++";
      break;
  } // switch

  // handle the figure suffix
  switch (figure->getFigureSuffixKind ()) {
    case msrFigure::k_NoFigureSuffix:
      break;
    case msrFigure::kDoubleFlatSuffix:
      s << "double flat";
      break;
    case msrFigure::kFlatSuffix:
      s << "flat";
      break;
    case msrFigure::kFlatFlatSuffix:
      s << "flat flat";
      break;
    case msrFigure::kNaturalSuffix:
      s << "natural";
      break;
    case msrFigure::kSharpSharpSuffix:
      s << "sharp sharp";
      break;
    case msrFigure::kSharpSuffix:
      s << "sharp";
      break;
    case msrFigure::kDoubleSharpSuffix:
      s << "souble sharp";
      break;
    case msrFigure::kSlashSuffix:
      s << "/";
      break;
  } // switch

  // generate a space if not last figure in figured bass
  if (
    fCurrentFiguredBassFiguresCounter
      <
    fCurrentFiguredBass->getFiguredBassFiguresList ().size ()
  ) {
    s << ' ';
  }

  return s.str ();
}

string lpsr2lilypondTranslator::figuredBassAsLilypondString (
  S_msrFiguredBass figuredBass)
{
  int inputLineNumber =
    figuredBass->getInputLineNumber ();

  stringstream s;

  s << "<";

  const list<S_msrFigure>&
    figuredBassFiguresList =
      figuredBass->getFiguredBassFiguresList ();

  if (figuredBassFiguresList.size ()) {
    list<S_msrFigure>::const_iterator
      iBegin = figuredBassFiguresList.begin (),
      iEnd   = figuredBassFiguresList.end (),
      i      = iBegin;

    for ( ; ; ) {
      s << figureAsLilypondString ((*i));
      if (++i == iEnd) break;
      s << " ";
    } // for
  }

  s << ">";

  // print figured bass duration
  msrTupletFactor
    figuredBassTupletFactor =
      figuredBass->getFiguredBassTupletFactor ();

  if (figuredBassTupletFactor.isEqualToOne ()) { // JMI ???
    // use figured bass sounding whole notes
    s <<
      durationAsLilypondString (
        inputLineNumber,
        figuredBass->
          getMeasureElementSoundingWholeNotes ());
  }
  else {
    // use figured bass display whole notes and tuplet factor
    s <<
      durationAsLilypondString (
        inputLineNumber,
        figuredBass->
          getFiguredBassDisplayWholeNotes ()) <<
      "*" <<
      figuredBassTupletFactor.asRational ();
  }

  s << ' ';

  return s.str ();
}

//________________________________________________________________________
bool compareFrameNotesByDecreasingStringNumber (
  const S_msrFrameNote& first,
  const S_msrFrameNote& second)
{
  return
    first->getFrameNoteStringNumber ()
      >
    second->getFrameNoteStringNumber ();
}

string lpsr2lilypondTranslator::frameAsLilypondString (
  S_msrFrame frame)
{
/* JMI
  int inputLineNumber =
    frame->getInputLineNumber ();
  */

  stringstream s;

  list<S_msrFrameNote>
    frameFrameNotesList =
      frame->getFrameFrameNotesList ();

  const list<msrBarre>&
    frameBarresList =
      frame->getFrameBarresList ();

  int frameStringsNumber =
    frame->getFrameStringsNumber ();
  int frameFretsNumber =
    frame->getFrameFretsNumber ();

  s <<
    "^\\markup {\\fret-diagram #\"";

  // are there fingerings?
  if (frame->getFrameContainsFingerings ()) {
    s <<
      "f:1;";
  }

  // strings number
  if (frameStringsNumber != 6) {
    s <<
      "w:" <<
      frameStringsNumber <<
      ";";
  }

  // frets number
  s <<
    "h:" <<
    frameFretsNumber <<
    ";";

  // frame barres
  if (frameBarresList.size ()) {
    list<msrBarre>::const_iterator
      iBegin = frameBarresList.begin (),
      iEnd   = frameBarresList.end (),
      i      = iBegin;

    for ( ; ; ) {
      msrBarre barre = (*i);

      s <<
        "c:" <<
        barre.fBarreStartString <<
        "-" <<
        barre.fBarreStopString <<
        "-" <<
        barre.fBarreFretNumber <<
        ";";

      if (++i == iEnd) break;
  // JMI    os << ";";
    } // for
  }

  // frame notes
  if (frameFrameNotesList.size ()) {
    // sort the frame notes,
    // necessary both for code generation
    // and the detection of muted (i.e. absent) frame notes
    frameFrameNotesList.sort (
      compareFrameNotesByDecreasingStringNumber);

    int currentStringNumber = frameStringsNumber;

    // generate the code
    list<S_msrFrameNote>::const_iterator
      iBegin = frameFrameNotesList.begin (),
      iEnd   = frameFrameNotesList.end (),
      i      = iBegin;

    for ( ; ; ) {
      S_msrFrameNote
        frameNote = (*i);

      int frameNoteStringNumber =
        frameNote->getFrameNoteStringNumber ();
      int frameNoteFretNumber =
        frameNote->getFrameNoteFretNumber ();
      int frameNoteFingering =
        frameNote->getFrameNoteFingering ();

      while (currentStringNumber > frameNoteStringNumber) {
        // currentStringNumber is missing,
        // hence it is a muted note
        s <<
          currentStringNumber <<
          "-x;";

        currentStringNumber--;
      }

      // generate code for the frame note
      s <<
        frameNoteStringNumber <<
        "-";

      if (frameNoteFretNumber == 0) {
        s <<
          "o";
      }
      else {
        s <<
          frameNoteFretNumber;
      }

      if (frameNoteFingering != -1) {
        s <<
          "-" <<
          frameNoteFingering;
      }

      s <<
        ";";

      currentStringNumber--;

      if (++i == iEnd) break;
  // JMI    os << ";";
    } // for

    // handling low-numbered muted notes
    while (currentStringNumber != 0) {
      // currentStringNumber is missing,
      // hence it is a muted note
      s <<
        currentStringNumber <<
        "-x;";

      currentStringNumber--;
    }
  }

  s <<
    "\" } ";

  return s.str ();
}

void lpsr2lilypondTranslator::generateInputLineNumberAndOrPositionInMeasureAsAComment (
  S_msrMeasureElement measureElement)
{
  fLilypondCodeStream <<
    "%{ ";

  if (gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()) {
    // print the input line number as a comment
    fLilypondCodeStream <<
      "line " << measureElement->getInputLineNumber () << " ";
  }

  if (gGlobalLpsr2lilypondOahGroup->getGeneratePositionsInMeasures ()) {
    // print the position in measure as a comment
    fLilypondCodeStream <<
      "pim: " <<
      measureElement->getMeasureElementPositionInMeasure () <<
      " ";
  }

  fLilypondCodeStream <<
    "%} ";
}

//________________________________________________________________________
string lpsr2lilypondTranslator::generateAColumnForMarkup (
  string           theString,
  markupColumnKind columnKind)
{
  stringstream s;

  switch (columnKind) {
    case markupColumnKindLeftAligned:
      s << "\\column { ";
      break;
    case markupColumnKindLeftACentered:
      s << "\\center-column { ";
      break;
  } // switch
  s << endl;

  list<string> chunksList;

  splitRegularStringAtEndOfLines (
    theString,
    chunksList);

  if (chunksList.size ()) {
    // generate a markup containing the chunks
    list<string>::const_iterator
      iBegin = chunksList.begin (),
      iEnd   = chunksList.end (),
      i      = iBegin;

    for ( ; ; ) {
      s <<
        "\\line { \"" << (*i) << "\" }";
      if (++i == iEnd) break;
    // JMI ???  s << ' ';
      s << endl;
    } // for

    s <<
      " } ";

    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
      s <<
        "% " <<
        singularOrPlural (
          chunksList.size (), "chunk", "chunks");
    }

    s << endl;
  }

  return s.str ();
}

//________________________________________________________________________
string lpsr2lilypondTranslator::generateMultilineMarkup (
  string           theString,
  markupColumnKind columnKind)
{
  stringstream s;

  s <<
    "\\markup { " <<
    generateAColumnForMarkup (
      theString,
      columnKind) <<
    " } " <<
    endl;

  return s.str ();
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrScore& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrScore" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // initial empty line in LilyPond code
  // to help copy/paste it
// JMI  fLilypondCodeStream << endl;
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrScore& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrScore" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // final empty line in LilyPond code
  // to help copy/paste it
  fLilypondCodeStream << endl;
}

    // names

string lpsr2lilypondTranslator::nameAsLilypondString (
  string name)
{
  string result;

  size_t endOfLineFound = name.find ("\n");

  if (endOfLineFound != string::npos) {
    result =
      generateMultilineMarkup (
        name,
        markupColumnKindLeftACentered); // JMI ???
  }
  else {
    result = "\"" + name + "\"";
  }

  return result;
}

string lpsr2lilypondTranslator::lpsrVarValAssocKindAsLilypondString (
  lpsrVarValAssoc::lpsrVarValAssocKind
    lilyPondVarValAssocKind)
{
  string result;

  switch (lilyPondVarValAssocKind) {
    // library

    case lpsrVarValAssoc::kLibraryVersion:
      result = "version";
      break;

    // MusicXML informations

    case lpsrVarValAssoc::kMusicXMLWorkNumber:
      result = "opus";
      break;
    case lpsrVarValAssoc::kMusicXMLWorkTitle:
      result = "title";
      break;
    case lpsrVarValAssoc::kMusicXMLOpus:
      result = "opus";
      break;
    case lpsrVarValAssoc::kMusicXMLMovementNumber:
      result = "movementNumber";
      break;
    case lpsrVarValAssoc::kMusicXMLMovementTitle:
      result = "subtitle";
      break;
    case lpsrVarValAssoc::kMusicXMLEncodingDate:
      result = "encodingDate";
      break;
    case lpsrVarValAssoc::kMusicXMLScoreInstrument:
      result = "scoreInstrument";
      break;
    case lpsrVarValAssoc::kMusicXMLMiscellaneousField:
      result = "miscellaneousField";
      break;

    // LilyPond informations

    case lpsrVarValAssoc::kLilypondDedication:
      result = "dedication";
      break;

    case lpsrVarValAssoc::kLilypondPiece:
      result = "piece";
      break;
    case lpsrVarValAssoc::kLilypondOpus:
      result = "opus";
      break;

    case lpsrVarValAssoc::kLilypondTitle:
      result = "title";
      break;
    case lpsrVarValAssoc::kLilypondSubTitle:
      result = "subtitle";
      break;
    case lpsrVarValAssoc::kLilypondSubSubTitle:
      result = "subsubtitle";
      break;

    case lpsrVarValAssoc::kLilypondInstrument:
      result = "instrument";
      break;
    case lpsrVarValAssoc::kLilypondMeter:
      result = "meter";
      break;

    case lpsrVarValAssoc::kLilypondTagline:
      result = "tagline";
      break;
    case lpsrVarValAssoc::kLilypondCopyright:
      result = "copyright";
      break;

    case lpsrVarValAssoc::kLilypondMyBreak:
      result = "myBreak";
      break;
    case lpsrVarValAssoc::kLilypondMyPageBreak:
      result = "myPageBreak";
      break;
    case lpsrVarValAssoc::kLilypondGlobal:
      result = "global";
      break;
  } // switch

  return result;
}

string lpsr2lilypondTranslator::lpsrVarValAssocAsLilypondString (
  S_lpsrVarValAssoc lpsrVarValAssoc,
  int               fieldNameWidth)
{
  stringstream s;

  s << left <<
    setw (fieldNameWidth) <<
    lpsrVarValAssocKindAsLilypondString (
      lpsrVarValAssoc->
        getLilyPondVarValAssocKind ()) <<
    " = ";

  msrFontStyleKind
    varValFontStyleKind =
      lpsrVarValAssoc->
        getVarValFontStyleKind ();

  bool italicIsNeeded = false;

  switch (varValFontStyleKind) {
    case kFontStyleNone:
      break;
    case kFontStyleNormal:
      break;
    case KFontStyleItalic:
      italicIsNeeded = true;
      break;
    } // switch

  msrFontWeightKind
    varValFontWeightKind =
      lpsrVarValAssoc->
        getVarValFontWeightKind ();

  bool boldIsNeeded = false;

  switch (varValFontWeightKind) {
    case kFontWeightNone:
      break;
    case kFontWeightNormal:
      break;
    case kFontWeightBold:
      boldIsNeeded = true;
      break;
    } // switch

  bool markupIsNeeded = italicIsNeeded || boldIsNeeded;

  if (markupIsNeeded) {
    fLilypondCodeStream << "\\markup { ";
  }

  if (italicIsNeeded) {
    fLilypondCodeStream << "\\italic ";
  }
  if (boldIsNeeded) {
    fLilypondCodeStream << "\\bold ";
  }

  s <<
    "\"" <<
    lpsrVarValAssoc->
      getVariableValue () <<
    "\"";

  if (markupIsNeeded) {
    s << " }";
  }

  return s.str ();
}

string lpsr2lilypondTranslator::lpsrVarValsListAssocKindAsLilypondString (
  lpsrVarValsListAssoc::lpsrVarValsListAssocKind
    lilyPondVarValsListAssocKind)
{
  string result;

  switch (lilyPondVarValsListAssocKind) {
    // MusicXML informations

    case lpsrVarValsListAssoc::kMusicXMLRights:
      result = "rights";
      break;
    case lpsrVarValsListAssoc::kMusicXMLComposer:
      result = "composer";
      break;
    case lpsrVarValsListAssoc::kMusicXMLArranger:
      result = "arranger";
      break;
    case lpsrVarValsListAssoc::kMusicXMLPoet:
      result = "poet";
      break;
    case lpsrVarValsListAssoc::kMusicXMLLyricist:
      result = "lyricist";
      break;
    case lpsrVarValsListAssoc::kMusicXMLTranslator:
      result = "translator";
      break;
    case lpsrVarValsListAssoc::kMusicXMLArtist:
      result = "artist";
      break;
    case lpsrVarValsListAssoc::kMusicXMLSoftware:
      result = "software";
      break;
    } // switch

  return result;
}

void lpsr2lilypondTranslator::generateLpsrVarValsListAssocValues (
  S_lpsrVarValsListAssoc varValsListAssoc)
{
  const list<string>&
    variableValuesList =
      varValsListAssoc->
        getVariableValuesList ();

  switch (variableValuesList.size ()) {
    case 0:
      break;

    case 1:
      // generate a single string
      fLilypondCodeStream <<
        "\"" <<
        escapeDoubleQuotes (variableValuesList.front ()) <<
        "\"";
      break;

    default:
      // generate a markup containing the chunks
      fLilypondCodeStream <<
        endl <<
        "\\markup {" <<
        endl;

      gIndenter++;

      fLilypondCodeStream <<
        "\\column {" <<
        endl;

      gIndenter++;

      list<string>::const_iterator
        iBegin = variableValuesList.begin (),
        iEnd   = variableValuesList.end (),
        i      = iBegin;

      for ( ; ; ) {
        fLilypondCodeStream <<
          "\"" << (*i) << "\"";
        if (++i == iEnd) break;
        fLilypondCodeStream << endl;
      } // for

      fLilypondCodeStream << endl;

      gIndenter--;

      fLilypondCodeStream <<
        "}" <<
        endl;

      gIndenter--;

      fLilypondCodeStream <<
        "}" <<
        endl;
  } // switch
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrVarValAssoc& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrVarValAssoc" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // generate the comment if needed
  string
    comment =
      elt->getComment ();

  if (comment.size ()) {
    fLilypondCodeStream <<
      "% " << comment <<
      endl;
  }

  // generate a comment out if needed
  switch (elt->getCommentedKind ()) {
    case lpsrVarValAssoc::kCommentedYes:
      fLilypondCodeStream << "%";
      break;
    case lpsrVarValAssoc::kCommentedNo:
      break;
  } // switch

  // generate the backslash if needed
  switch (elt->getBackSlashKind ()) {
    case lpsrVarValAssoc::kWithBackSlashYes:
      fLilypondCodeStream << "\\";
      break;
    case lpsrVarValAssoc::kWithBackSlashNo:
      break;
  } // switch

  lpsrVarValAssoc::lpsrVarValAssocKind
    varValAssocKind =
      elt->getLilyPondVarValAssocKind ();

  string
    lilyPondVarValAssocKindAsLilypondString =
      lpsrVarValAssocKindAsLilypondString (
        varValAssocKind);

  // the largest variable name length in a header is 18 JMI
  int fieldWidth;

  if (fOnGoingHeader) {
    fieldWidth = 18;
  }
  else {
    fieldWidth =
      lilyPondVarValAssocKindAsLilypondString.size ();
  }

  // generate the field name
  fLilypondCodeStream << left<<
    setw (fieldWidth) <<
    lilyPondVarValAssocKindAsLilypondString;

  switch (elt->getVarValSeparatorKind ()) {
    case lpsrVarValAssoc::kVarValSeparatorSpace:
      fLilypondCodeStream << ' ';
      break;
    case lpsrVarValAssoc::kVarValSeparatorEqualSign:
      fLilypondCodeStream << " = ";
      break;
  } // switch

  msrFontStyleKind
    varValFontStyleKind =
      elt->getVarValFontStyleKind ();

  bool italicIsNeeded = false;

  switch (varValFontStyleKind) {
    case kFontStyleNone:
      break;
    case kFontStyleNormal:
      break;
    case KFontStyleItalic:
      italicIsNeeded = true;
      break;
    } // switch

  msrFontWeightKind
    varValFontWeightKind =
      elt->getVarValFontWeightKind ();

  bool boldIsNeeded = false;

  switch (varValFontWeightKind) {
    case kFontWeightNone:
      break;
    case kFontWeightNormal:
      break;
    case kFontWeightBold:
      boldIsNeeded = true;
      break;
    } // switch

  bool markupIsNeeded = italicIsNeeded || boldIsNeeded;

  if (markupIsNeeded) {
    fLilypondCodeStream << "\\markup { ";
  }

  if (italicIsNeeded) {
    fLilypondCodeStream << "\\italic ";
  }
  if (boldIsNeeded) {
    fLilypondCodeStream << "\\bold ";
  }

  // generate the quote if needed
  lpsrVarValAssoc::lpsrQuotesKind
    quotesKind =
      elt->getQuotesKind ();

  switch (quotesKind) {
    case lpsrVarValAssoc::kQuotesAroundValueYes:
      fLilypondCodeStream << "\"";
      break;
    case lpsrVarValAssoc::kQuotesAroundValueNo:
      break;
  } // switch

  // generate the value and unit if any
  if (elt->getUnit ().size ()) {
    fLilypondCodeStream <<
      setprecision (2) <<
      elt->getVariableValue ();

    fLilypondCodeStream <<
      "\\" <<
      elt->getUnit ();
  }
  else {
    fLilypondCodeStream <<
      elt->getVariableValue ();
  }

  // generate the quote if needed
  switch (quotesKind) {
    case lpsrVarValAssoc::kQuotesAroundValueYes:
      fLilypondCodeStream << "\"";
      break;
    case lpsrVarValAssoc::kQuotesAroundValueNo:
      break;
  } // switch

  if (markupIsNeeded) {
    fLilypondCodeStream << " }";
  }

  fLilypondCodeStream << endl;

  // generate the end line(s) if needed
  switch (elt->getEndlKind ()) {
    case lpsrVarValAssoc::kEndlNone:
      break;
    case lpsrVarValAssoc::kEndlOnce:
      fLilypondCodeStream << endl;
      break;
    case lpsrVarValAssoc::kEndlTwice:
      fLilypondCodeStream <<
        endl << endl;
      break;
  } // switch
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrVarValAssoc& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrVarValAssoc" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrVarValsListAssoc& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrVarValsListAssoc" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // the largest variable name length in a header is 18 JMI
  int fieldWidth;

  string
    lilyPondVarValsListAssocKindAsString =
      elt->lilyPondVarValsListAssocKindAsString ();

  if (fOnGoingHeader) {
    fieldWidth = 18;
  }
  else {
    fieldWidth =
      lilyPondVarValsListAssocKindAsString.size ();
  }

  fLilypondCodeStream << left<<
    setw (fieldWidth) <<
    lpsrVarValsListAssocKindAsLilypondString (
      elt->getVarValsListAssocKind ());

  fLilypondCodeStream << " = ";

  generateLpsrVarValsListAssocValues (elt);

  fLilypondCodeStream << endl;
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrVarValsListAssoc& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrVarValsListAssoc" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrSchemeVariable& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrSchemeVariable" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  string
    comment =
      elt->getComment ();

  if (comment.size ()) {
    fLilypondCodeStream <<
      "% " << comment <<
      endl;
  }

  switch (elt->getCommentedKind ()) {
    case lpsrSchemeVariable::kCommentedYes:
      fLilypondCodeStream << "% ";
      break;
    case lpsrSchemeVariable::kCommentedNo:
      break;
  } // switch

  fLilypondCodeStream <<
    "#(" <<
    elt->getVariableName () <<
    ' ' <<
    elt->getVariableValue () <<
    ")";

  switch (elt->getEndlKind ()) {
    case lpsrSchemeVariable::kEndlNone:
      break;
    case lpsrSchemeVariable::kEndlOnce:
      fLilypondCodeStream << endl;
      break;
    case lpsrSchemeVariable::kEndlTwice:
      fLilypondCodeStream <<
        endl << endl;
      break;
  } // switch
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrSchemeVariable& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrSchemeVariable" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//______________________________________________________________________________
string lpsr2lilypondTranslator::lengthUnitAsLilypondString (
  msrLengthUnitKind lengthUnitKind)
{
  string result;

  switch (lengthUnitKind) {
    case kInchUnit:
      result = "\\in";
      break;
    case kCentimeterUnit:
      result = "\\cm";
      break;
    case kMillimeterUnit:
      result = "\\mm";
      break;
  } // switch

  return result;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrHeader& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrHeader" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (gGlobalLpsr2lilypondOahGroup->getNoHeaderBlock ()) {
    return;
  }

  fLilypondCodeStream <<
    "\\header {" <<
    endl;

  gIndenter++; // decremented in visitEnd (S_lpsrHeader&)

  // generate header elements

  int fieldNameWidth =
    elt->maxLilypondVariablesNamesLength ();

  // MusicXML informations JMI ???

  // LilyPond informations

  {
    // dedication
    S_lpsrVarValAssoc
      lilypondDedication =
        elt->getLilypondDedication ();

    if (lilypondDedication) {
      fLilypondCodeStream <<
        lpsrVarValAssocAsLilypondString (
          lilypondDedication,
          fieldNameWidth) <<
        endl;
    }
  }

  {
    // piece
    S_lpsrVarValAssoc
      lilypondPiece =
        elt->getLilypondPiece ();

    if (lilypondPiece) {
      fLilypondCodeStream <<
        lpsrVarValAssocAsLilypondString (
          lilypondPiece,
          fieldNameWidth) <<
        endl;
    }
  }

  {
    // opus
    S_lpsrVarValAssoc
      lilypondOpus =
        elt->getLilypondOpus ();

    if (lilypondOpus) {
      fLilypondCodeStream <<
        lpsrVarValAssocAsLilypondString (
          lilypondOpus,
          fieldNameWidth) <<
        endl;
    }
  }

  {
    // title
    if (gGlobalMxmlTree2msrOahGroup->getUseFilenameAsWorkTitle ()) {
      fLilypondCodeStream << left <<
        setw (fieldNameWidth) <<
        "title = \"" <<
        gGlobalOahOahGroup->getInputSourceName () <<
        "\"" <<
        endl;
    }
    else {
      S_lpsrVarValAssoc
        lilypondTitle =
          elt->getLilypondTitle ();

      if (lilypondTitle) {
        fLilypondCodeStream <<
          lpsrVarValAssocAsLilypondString (
            lilypondTitle,
            fieldNameWidth) <<
          endl;
      }
    }

    /* JMI
    S_msrLength
      indent =
        elt->getIndent ();

    if (! indent) {
      fLilypondCodeStream << "%";
    }
    fLilypondCodeStream << left <<
      setw (fieldWidth) <<
      "indent" << " = ";
    if (indent) {
      fLilypondCodeStream <<
        setprecision (3) << indent->getLengthValue () <<
        lengthUnitAsLilypondString (indent->getLengthUnitKind ());
    }
    else {
      fLilypondCodeStream <<
        "0.0" <<
        lengthUnitAsLilypondString (defaultLengthUnit);
    }
    fLilypondCodeStream << endl;
*/

    // subtitle
    S_lpsrVarValAssoc
      lilypondSubTitle =
        elt->getLilypondSubTitle ();

    if (lilypondSubTitle) {
      fLilypondCodeStream <<
        lpsrVarValAssocAsLilypondString (
          lilypondSubTitle,
          fieldNameWidth) <<
        endl;
    }

    S_lpsrVarValAssoc
      lilypondSubSubTitle =
        elt->getLilypondSubSubTitle ();

    if (lilypondSubSubTitle) {
      fLilypondCodeStream <<
        lpsrVarValAssocAsLilypondString (
          lilypondSubSubTitle,
          fieldNameWidth) <<
        endl;
    }
  }

  {
    // instrument
    S_lpsrVarValAssoc
      lilypondInstrument =
        elt->getLilypondInstrument ();

    if (lilypondInstrument) {
      fLilypondCodeStream <<
        lpsrVarValAssocAsLilypondString (
          lilypondInstrument,
          fieldNameWidth) <<
        endl;
    }

    S_lpsrVarValAssoc
      lilypondMeter =
        elt->getLilypondMeter ();

    if (lilypondMeter) {
      fLilypondCodeStream <<
        lpsrVarValAssocAsLilypondString (
          lilypondMeter,
          fieldNameWidth) <<
        endl;
    }
  }

  {
    // copyright
    S_lpsrVarValAssoc
      lilypondCopyright =
        elt->getLilypondCopyright ();

    if (lilypondCopyright) {
      fLilypondCodeStream <<
        lpsrVarValAssocAsLilypondString (
          lilypondCopyright,
          fieldNameWidth) <<
        endl;
    }

    S_lpsrVarValAssoc
      lilypondTagline =
        elt->getLilypondTagline ();

    if (lilypondTagline) {
      fLilypondCodeStream <<
        lpsrVarValAssocAsLilypondString (
          lilypondTagline,
          fieldNameWidth) <<
        endl;
    }
  }

  fOnGoingHeader = true;
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrHeader& elt)
{
  if (gGlobalLpsr2lilypondOahGroup->getNoHeaderBlock ()) {
    return;
  }

  gIndenter--; // incremented in visitStart (S_lpsrHeader&)

#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrHeader" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    "}";

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      " % header";
  }

  fLilypondCodeStream <<
    endl << endl;

  fOnGoingHeader = false;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generatePaperPageSize (
  S_msrPageLayout   pageLayout,
  msrLengthUnitKind defaultLengthUnit,
  int               fieldWidth)
{
  // paper height
  msrLength
    paperHeight;

//  if (gGlobalLpsrOahGroup->getPaperHeight ().getLengthValue () >= 0.0) { // JMI BLARK
  if (gGlobalLpsrOahGroup->getPaperHeightAtom ()->getVariableHasBeenSet ()) {
    paperHeight =
      gGlobalLpsrOahGroup->getPaperHeight ();
  }
  else if (pageLayout) {
    paperHeight =
      * (pageLayout->getPageHeight ()); // BLARK
  }

  if (paperHeight.getLengthValue () >= 0.0) { // JMI ???
    if (paperHeight.getLengthValue () < 0.0) {
      fLilypondCodeStream << "%";
    }
    fLilypondCodeStream << left <<
      setw (fieldWidth) <<
      "paper-height" << " = ";
    if (false && paperHeight.getLengthValue () >= 0.0) { // JMI
      fLilypondCodeStream <<
        setprecision (3) << paperHeight.getLengthValue () <<
        lengthUnitAsLilypondString (paperHeight.getLengthUnitKind ());
    }
    else {
      fLilypondCodeStream <<
        "297.0" <<
        lengthUnitAsLilypondString (defaultLengthUnit);
    }
    fLilypondCodeStream << endl;
  }

  // paper width
  msrLength
    paperWidth;

//  if (gGlobalLpsrOahGroup->getPaperWidth ().getLengthValue () >= 0.0) { // JMI BLARK
  if (gGlobalLpsrOahGroup->getPaperWidthAtom ()->getVariableHasBeenSet ()) {
    paperWidth =
      gGlobalLpsrOahGroup->getPaperWidth ();
  }
  else if (pageLayout) {
    paperWidth =
      * (pageLayout->getPageWidth ()); // BLARK
  }

  if (paperWidth.getLengthValue () >= 0.0) {
    if (paperWidth.getLengthValue () < 0.0) {
      fLilypondCodeStream << "%";
    }
    fLilypondCodeStream << left <<
      setw (fieldWidth) <<
      "paper-width" << " = ";
    if (false && paperWidth.getLengthValue () >= 0.0) { // JMI
      fLilypondCodeStream <<
        setprecision (3) << paperWidth.getLengthValue () <<
        lengthUnitAsLilypondString (paperWidth.getLengthUnitKind ());
    }
    else {
      fLilypondCodeStream <<
        "210.0" <<
        lengthUnitAsLilypondString (defaultLengthUnit);
    }
    fLilypondCodeStream << endl;
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generatePaperMargins (
  S_msrPageLayout   pageLayout,
  msrLengthUnitKind defaultLengthUnit,
  int               fieldWidth)
{
  return; // JMI
  // left margin
  bool   commentOutLeftMargin = false;
  float  leftMarginValue = 0.0; // JMI
  string leftMarginUnitString =
           lengthUnitAsLilypondString (defaultLengthUnit);

  if (pageLayout) {
    S_msrMargin
      leftMargin =
        pageLayout->getSingleLeftMargin ();

    if (leftMargin) {
      leftMarginValue =
        leftMargin->getMarginLength ().getLengthValue ();

      leftMarginUnitString =
        lengthUnitAsLilypondString (
          leftMargin->getMarginLength ().getLengthUnitKind ());
    }
    else {
      commentOutLeftMargin = true;
    }
  }
  else {
    commentOutLeftMargin = true;
  }

  if (commentOutLeftMargin) {
    fLilypondCodeStream << "%";
  }
  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "left-margin" << " = " <<
    setprecision (3) << leftMarginValue <<
    leftMarginUnitString <<
    endl;

  // right margin
  bool   commentOutRightMargin = false;
  float  rightMarginValue = 0.0; // JMI
  string rightMarginUnitString =
           lengthUnitAsLilypondString (defaultLengthUnit);

  if (pageLayout) {
    S_msrMargin
      rightMargin =
        pageLayout->getSingleRightMargin ();

    if (rightMargin) {
      rightMarginValue =
        rightMargin->getMarginLength ().getLengthValue ();

      rightMarginUnitString =
        lengthUnitAsLilypondString (
          rightMargin->getMarginLength ().getLengthUnitKind ());
    }
    else {
      commentOutRightMargin = true;
    }
  }
  else {
    commentOutRightMargin = true;
  }

  if (commentOutRightMargin) {
    fLilypondCodeStream << "%";
  }
  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "right-margin" << " = " <<
    setprecision (3) << rightMarginValue <<
    rightMarginUnitString <<
    endl;

  // top margin
  bool   commentOutTopMargin = false;
  float  topMarginValue = 0.0; // JMI
  string topMarginUnitString =
           lengthUnitAsLilypondString (defaultLengthUnit);

  if (pageLayout) {
    S_msrMargin
      topMargin =
        pageLayout->getSingleTopMargin ();

    if (topMargin) {
      topMarginValue =
        topMargin->getMarginLength ().getLengthValue ();

      topMarginUnitString =
        lengthUnitAsLilypondString (
          topMargin->getMarginLength ().getLengthUnitKind ());
    }
    else {
      commentOutTopMargin = true;
    }
  }
  else {
    commentOutTopMargin = true;
  }

  if (commentOutTopMargin) {
    fLilypondCodeStream << "%";
  }
  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "top-margin" << " = " <<
    setprecision (3) << topMarginValue <<
    topMarginUnitString <<
    endl;

  // bottom margin
  bool   commentOutBottomMargin = false;
  float  bottomMarginValue = 0.0; // JMI
  string bottomMarginUnitString =
           lengthUnitAsLilypondString (defaultLengthUnit);

  if (pageLayout) {
    S_msrMargin
      bottomMargin =
        pageLayout->getSingleBottomMargin ();

    if (bottomMargin) {
      bottomMarginValue =
        bottomMargin->getMarginLength ().getLengthValue ();

      bottomMarginUnitString =
        lengthUnitAsLilypondString (
          bottomMargin->getMarginLength ().getLengthUnitKind ());
    }
    else {
      commentOutBottomMargin = true;
    }
  }
  else {
    commentOutBottomMargin = true;
  }

  if (commentOutBottomMargin) {
    fLilypondCodeStream << "%";
  }
  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "bottom-margin" << " = " <<
    setprecision (3) << bottomMarginValue <<
    bottomMarginUnitString <<
    endl;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generatePaperIndents (
  S_lpsrPaper       pagePaper,
  msrLengthUnitKind defaultLengthUnit,
  int               fieldWidth)
{
  return; // JMI
  // horizontal shift
  S_msrLength
    horizontalShift =
      pagePaper->getHorizontalShift ();

  if (! horizontalShift) {
    fLilypondCodeStream << "%";
  }
  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "horizontal-shift" << " = ";
  if (horizontalShift) {
    fLilypondCodeStream <<
      setprecision (3) << horizontalShift->getLengthValue () <<
      lengthUnitAsLilypondString (horizontalShift->getLengthUnitKind ());
  }
  else {
    fLilypondCodeStream <<
      "0.0" <<
      lengthUnitAsLilypondString (defaultLengthUnit);
  }
  fLilypondCodeStream << endl;

  // indent
  S_msrLength
    indent =
      pagePaper->getIndent ();

  if (! indent) {
    fLilypondCodeStream << "%";
  }
  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "indent" << " = ";
  if (indent) {
    fLilypondCodeStream <<
      setprecision (3) << indent->getLengthValue () <<
      lengthUnitAsLilypondString (indent->getLengthUnitKind ());
  }
  else {
    fLilypondCodeStream <<
      "0.0" <<
      lengthUnitAsLilypondString (defaultLengthUnit);
  }
  fLilypondCodeStream << endl;

  // short indent
  S_msrLength
    shortIndent =
      pagePaper->getShortIndent ();

  if (! shortIndent) {
    fLilypondCodeStream << "%";
  }
  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "short-indent" << " = ";
  if (shortIndent) {
    fLilypondCodeStream <<
      setprecision (3) << shortIndent->getLengthValue () <<
      lengthUnitAsLilypondString (shortIndent->getLengthUnitKind ());
  }
  else {
    fLilypondCodeStream <<
      "0.0" <<
      lengthUnitAsLilypondString (defaultLengthUnit);
  }
  fLilypondCodeStream << endl;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generatePaperSpaces (
  S_lpsrPaper       pagePaper,
  msrLengthUnitKind defaultLengthUnit,
  int               fieldWidth)
{
  return; // JMI
  // markup system spacing padding
  S_msrLength
    markupSystemPpacingPadding =
      pagePaper->getMarkupSystemSpacingPadding ();

  if (! markupSystemPpacingPadding) {
    fLilypondCodeStream << "%";
  }
  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "markup-system-spacing-padding" << " = ";
  if (markupSystemPpacingPadding) {
    fLilypondCodeStream <<
      setprecision (3) << markupSystemPpacingPadding->getLengthValue () <<
      lengthUnitAsLilypondString (markupSystemPpacingPadding->getLengthUnitKind ());
  }
  else {
    fLilypondCodeStream <<
      "0.0" <<
      lengthUnitAsLilypondString (defaultLengthUnit);
  }
  fLilypondCodeStream << endl;

  // between system space
  S_msrLength
    betweenSystemSpace =
      pagePaper->getBetweenSystemSpace ();

  if (! betweenSystemSpace) {
    fLilypondCodeStream << "%";
  }
  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "between-system-space" << " = ";
  if (betweenSystemSpace) {
    fLilypondCodeStream <<
      setprecision (3) << betweenSystemSpace->getLengthValue () <<
      lengthUnitAsLilypondString (betweenSystemSpace->getLengthUnitKind ());
  }
  else {
    fLilypondCodeStream <<
      "0.0" <<
      lengthUnitAsLilypondString (defaultLengthUnit);
  }
  fLilypondCodeStream << endl;

  // page top space
  S_msrLength
    pageTopSpace =
      pagePaper->getPageTopSpace ();

  if (! pageTopSpace) {
    fLilypondCodeStream << "%";
  }
  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "page-top-space" << " = ";
  if (pageTopSpace) {
    fLilypondCodeStream <<
      setprecision (3) << pageTopSpace->getLengthValue () <<
      lengthUnitAsLilypondString (pageTopSpace->getLengthUnitKind ());
  }
  else {
    fLilypondCodeStream <<
      "0.0" <<
      lengthUnitAsLilypondString (defaultLengthUnit);
  }
  fLilypondCodeStream << endl;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generatePaperCounts (
  S_lpsrPaper       pagePaper,
  msrLengthUnitKind defaultLengthUnit,
  int               fieldWidth)
{
  return; // JMI
  // page count
  int
    pageCount =
      pagePaper->getPageCount ();

  if (pageCount < 0) {
    fLilypondCodeStream << "%";
  }
  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "page-count" << " = " <<
     pageCount <<
     endl;

  // system count
  int
    systemCount =
      pagePaper->getSystemCount ();

  if (systemCount < 0) {
    fLilypondCodeStream << "%";
  }
  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "system-count" << " = " <<
     systemCount <<
     endl;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generatePaperHeadersAndFooters (
  S_lpsrPaper       pagePaper,
  msrLengthUnitKind defaultLengthUnit,
  int               fieldWidth)
{
  return; // JMI
  string oddHeaderMarkup =
    pagePaper->getOddHeaderMarkup ();

  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "oddHeaderMarkup" << " = ";
  if (oddHeaderMarkup.size ()) {
    fLilypondCodeStream <<
      oddHeaderMarkup;
  }
  else {
    fLilypondCodeStream << "\"\"";
  }
  fLilypondCodeStream << endl;

  string evenHeaderMarkup =
    pagePaper->getEvenHeaderMarkup ();

  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "evenHeaderMarkup" << " = ";
  if (evenHeaderMarkup.size ()) {
    fLilypondCodeStream <<
      evenHeaderMarkup;
  }
  else {
    fLilypondCodeStream << "\"\"";
  }
  fLilypondCodeStream << endl;

  string oddFooterMarkup =
    pagePaper->getOddFooterMarkup ();

  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "oddFooterMarkup" << " = ";
  if (oddFooterMarkup.size ()) {
    fLilypondCodeStream <<
      oddFooterMarkup;
  }
  else {
    fLilypondCodeStream << "\"\"";
  }
  fLilypondCodeStream << endl;

  string evenFooterMarkup =
    pagePaper->getEvenFooterMarkup ();

  fLilypondCodeStream << left <<
    setw (fieldWidth) <<
    "evenFooterMarkup" << " = ";
  if (evenFooterMarkup.size ()) {
    fLilypondCodeStream <<
      evenFooterMarkup;
  }
  else {
    fLilypondCodeStream << "\"\"";
  }
  fLilypondCodeStream << endl;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrPaper& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrPaper" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (gGlobalLpsr2lilypondOahGroup->getNoPaperBlock ()) {
    return;
  }

/*
  we could generate the LPSR paper code a second time
  with an option JMI ???
*/

  if (! fOnGoingBookPartBlock) {
    // get MSR page layout
    S_msrPageLayout
      pageLayout =
        elt->getPageLayout ();

    // default length unit
    const msrLengthUnitKind
      defaultLengthUnit = kMillimeterUnit; // JMI

    fLilypondCodeStream <<
      "\\paper" << " {" <<
      endl;

    gIndenter++; // decremented in visitEnd (S_lpsrPaper& elt)

    const int fieldWidth = 30;

    // page size
    generatePaperPageSize (
      pageLayout,
      defaultLengthUnit,
      fieldWidth);

    // separator
    fLilypondCodeStream << endl;

    // margins
    generatePaperMargins (
      pageLayout,
      defaultLengthUnit,
      fieldWidth);

    // separator
    fLilypondCodeStream << endl;

    // indents
    generatePaperIndents (
      elt,
      defaultLengthUnit,
      fieldWidth);

    // separator
    fLilypondCodeStream << endl;

    // spaces
    generatePaperSpaces (
      elt,
      defaultLengthUnit,
      fieldWidth);

    // separator
    fLilypondCodeStream << endl;

    // counts
    generatePaperCounts (
      elt,
      defaultLengthUnit,
      fieldWidth);

    // separator
    fLilypondCodeStream << endl;

    // headers and footers
    generatePaperHeadersAndFooters (
      elt,
      defaultLengthUnit,
      fieldWidth);
  }
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrPaper& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrPaper" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (gGlobalLpsr2lilypondOahGroup->getNoPaperBlock ()) {
    return;
  }

  if (! fOnGoingBookPartBlock) {
    gIndenter--; // incremented in visitStart (S_lpsrPaper& elt)

    fLilypondCodeStream <<
      "}";

    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
      fLilypondCodeStream << left <<
        setw (commentFieldWidth) <<
        " % paper";
    }

    fLilypondCodeStream <<
      endl << endl;
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrLayout& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrLayout" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    "\\layout {" <<
    endl;

  gIndenter++; // decremented in visitEnd (S_lpsrLayout& elt)

  // score context?
  if (! gGlobalLpsr2lilypondOahGroup->getNoLayoutScoreContext ()) {
    fLilypondCodeStream <<
      "\\context {" <<
      endl <<
      gTab << "\\Score" <<
      endl <<
      gTab << "autoBeaming = ##f % to display tuplets brackets" <<
      endl <<
      "}";

    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
      fLilypondCodeStream << left <<
        setw (commentFieldWidth) <<
        " % score context";
      }

    fLilypondCodeStream << endl;
  }

  // voice context?
  if (! gGlobalLpsr2lilypondOahGroup->getNoLayoutVoiceContext ()) {
    fLilypondCodeStream <<
      "\\context {" <<
      endl <<
      gTab << "\\Voice" <<
      endl;
  }

  if (gGlobalLpsr2lilypondOahGroup->getAmbitusEngraver ()) {
    fLilypondCodeStream <<
        gTab << "\\consists \"Ambitus_engraver\"" <<
        endl;
  }

  if (gGlobalLpsr2lilypondOahGroup->getCustosEngraver ()) {
    fLilypondCodeStream <<
        gTab << "\\consists \"Custos_engraver\"" <<
        endl;
  }

  if (! gGlobalLpsr2lilypondOahGroup->getNoLayoutVoiceContext ()) {
    fLilypondCodeStream <<
      "}";


    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
      fLilypondCodeStream << left <<
        setw (commentFieldWidth) <<
        " % voice context";
      }

    fLilypondCodeStream << endl;
  }

  // ChordNames context
  if (fVisitedLpsrScore->getJazzChordsDisplayIsNeeded ()) {
    fLilypondCodeStream <<
      gIndenter.indentMultiLineString (
R"(\context {
  \ChordNames
  chordNameExceptions = \chExceptions
  slashChordSeparator = \markup {
    % the \hspace commands simulate kerning
    \hspace #-.7
    \fontsize #1 \lower #1.2 \rotate #-10 "/"
    \hspace #-.1
  }
  chordNoteNamer = #lower-extension
})") <<
      endl;
  }
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrLayout& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrLayout" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (gGlobalLpsr2lilypondOahGroup->getRepeatBrackets ()) {
    fLilypondCodeStream <<
      "\\context " "{" <<
      endl;

    gIndenter++;

    fLilypondCodeStream <<
      "\\Score" <<
      endl <<
      "% defaultBarType = #\"!\"" <<
      endl <<
      "startRepeatType = #\"[|:\"" <<
      endl <<
      "endRepeatType = #\":|]\"" <<
      endl <<
      "doubleRepeatType = #\":|][|:\"" <<
      endl;

    gIndenter--;

    fLilypondCodeStream <<
      "}" <<
      endl;
  }

  if (false) { // JMI XXL
    fLilypondCodeStream <<
      "\\context {" <<
      endl;

    gIndenter++;

    fLilypondCodeStream <<
      "\\Staff" <<
      endl <<
      "\\consists \"Span_arpeggio_engraver\"" <<
      endl;

    gIndenter--;

    fLilypondCodeStream <<
      "}" " % staff contact";

    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
      fLilypondCodeStream << left <<
        setw (commentFieldWidth) <<
        " % layout";
    }

    fLilypondCodeStream << endl;
  }

  gIndenter--; // incremented in visitStart (S_lpsrLayout& elt)

  fLilypondCodeStream <<
    "}";

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      " % layout";
  }

  fLilypondCodeStream <<
    endl << endl;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrBookBlock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrBookBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (gGlobalLpsr2lilypondOahGroup->getNoBookBlock ()) {
    return;
  }

  fLilypondCodeStream <<
    "\\book {" <<
    endl;

  gIndenter++; // decremented in visitEnd (S_lpsrBookBlock& elt)

  fOnGoingBookPartBlock = true;
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrBookBlock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrBookBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (gGlobalLpsr2lilypondOahGroup->getNoBookBlock ()) {
    return;
  }

  gIndenter--; // incremented in visitStart (S_lpsrBookBlock& elt)

  fLilypondCodeStream <<
    "}";

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      " % book";
  }

  fLilypondCodeStream << endl;

  fOnGoingBookPartBlock = false;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrScoreBlock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrScoreBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    "\\score {" <<
    endl;

  gIndenter++; // decremented in visitEnd (S_lpsrScoreBlock& elt)

  fOnGoingScoreBlock = true;
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrScoreBlock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrScoreBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  gIndenter--; // incremented in visitStart (S_lpsrScoreBlock& elt)

  fLilypondCodeStream <<
    "}";

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      " % score";
  }

  fLilypondCodeStream <<
    endl << // JMI
    endl;

  fOnGoingScoreBlock = false;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrBookPartBlock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrBookPartBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    "\\bookpart {" <<
    endl;

  gIndenter++; //  // decremented in visitEnd (S_lpsrBookPartBlock& elt)

  fOnGoingBookPartBlock = true;
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrBookPartBlock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrBookPartBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  gIndenter--; // incremented in visitStart (S_lpsrBookPartBlock& elt)

  fLilypondCodeStream <<
    "}";

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      " % bookpart";
  }

  fLilypondCodeStream <<
    endl << endl;

  fOnGoingBookPartBlock = false;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrParallelMusicBLock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrParallelMusicBLock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fNumberOfPartGroupBlocks =
    elt->
      getParallelMusicBLockPartGroupBlocks ().size ();

  if (fNumberOfPartGroupBlocks > 0) {
    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
      fLilypondCodeStream << left <<
        setw (commentFieldWidth) <<
        " <<" <<
        "% parallel music";
    }

    else {
      fLilypondCodeStream <<
        " <<";
    }

    fLilypondCodeStream << endl;

    gIndenter++; //  // decremented in visitEnd (S_lpsrParallelMusicBLock& elt)
  }

  fCurrentParallelMusicBLock = elt;
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrParallelMusicBLock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrParallelMusicBLock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream << endl;

  if (fNumberOfPartGroupBlocks > 0) {
    gIndenter--; // incremented in visitStart (S_lpsrParallelMusicBLock& elt)

    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
      fLilypondCodeStream << left <<
        setw (commentFieldWidth) <<
        ">>" <<
        "% parallel music";
    }

    else {
      fLilypondCodeStream <<
        ">>";
    }

    fLilypondCodeStream << endl << endl;
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrPartGroupBlock& elt)
{
  // fetch part group
  S_msrPartGroup
    partGroup =
      elt->getPartGroup ();

#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrPartGroupBlock for '" <<
      partGroup->asShortString () <<
      "'" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fPartGroupBlocksCounter++;

  fNumberOfPartGroupBlockElements =
    elt -> getPartGroupBlockElements ().size ();

// JMI  fLilypondCodeStream << endl << endl << partGroup << endl << endl;

  msrPartGroup::msrPartGroupImplicitKind
    partGroupImplicitKind =
      partGroup->
        getPartGroupImplicitKind ();

  msrPartGroup::msrPartGroupSymbolKind
    partGroupSymbolKind =
      partGroup->
        getPartGroupSymbolKind ();

  msrPartGroup::msrPartGroupBarlineKind
    partGroupBarlineKind =
      partGroup->
        getPartGroupBarlineKind ();

  string
    partGroupName =
      partGroup->
        getPartGroupName (),

    partGroupAbbreviation =
      partGroup->
        getPartGroupAbbreviation (),

    partGroupInstrumentName =
      partGroup->
        getPartGroupInstrumentName ();

  // LPNR, page 567 jMI ???

  switch (partGroupImplicitKind) {
    case msrPartGroup::kPartGroupImplicitYes:
      // don't generate code for an implicit top-most part group block
      break;

    case msrPartGroup::kPartGroupImplicitNo:
      if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
        fLilypondCodeStream << left <<
          setw (commentFieldWidth);
      }

      switch (partGroupSymbolKind) {
        case msrPartGroup::kPartGroupSymbolNone:
          fLilypondCodeStream <<
            "\\new StaffGroup";
          break;

        case msrPartGroup::kPartGroupSymbolBrace: // JMI
          switch (partGroupBarlineKind) {
            case msrPartGroup::kPartGroupBarlineYes:
              fLilypondCodeStream <<
                "\\new PianoStaff";
              break;
            case msrPartGroup::kPartGroupBarlineNo:
              fLilypondCodeStream <<
                "\\new GrandStaff";
              break;
          } // switch
          break;

        case msrPartGroup::kPartGroupSymbolBracket:
          switch (partGroupBarlineKind) {
            case msrPartGroup::kPartGroupBarlineYes:
              fLilypondCodeStream <<
                "\\new StaffGroup";
              break;
            case msrPartGroup::kPartGroupBarlineNo:
              fLilypondCodeStream <<
                "\\new ChoirStaff";
              break;
          } // switch
          break;

        case msrPartGroup::kPartGroupSymbolLine:
          fLilypondCodeStream <<
            "\\new StaffGroup";
          break;

        case msrPartGroup::kPartGroupSymbolSquare:
          fLilypondCodeStream <<
            "\\new StaffGroup";
          break;
      } // switch

#ifdef TRACING_IS_ENABLED
      if (gGlobalTraceOahGroup->getTracePartGroups ()) {
         fLilypondCodeStream <<
          " %{ " <<
          partGroup->getPartGroupCombinedName () <<
          ", abs number: " <<
          partGroup->getPartGroupAbsoluteNumber () <<
          " %} ";
      }
#endif

      // should a '\with' block be generated?
      bool doGenerateAWithBlock = false;

      // generate the '\with' block if there's
      // a part group name or abbreviation to be generated
      if (
        partGroupName.size ()
          ||
        partGroupAbbreviation.size ()
      ) {
        doGenerateAWithBlock = true;
      }

      // generate the '\with' block
      // if the part group is not implicit
      switch (partGroupImplicitKind) {
        case msrPartGroup::kPartGroupImplicitYes:
          break;
        case msrPartGroup::kPartGroupImplicitNo:
          if (partGroupName.size ()) {
            doGenerateAWithBlock = true;
          }
          break;
      } // switch

      // generate the '\with' block
      // if the part group symbol is a line or square
      switch (partGroupSymbolKind) {
        case msrPartGroup::kPartGroupSymbolNone:
          break;

        case msrPartGroup::kPartGroupSymbolBrace: // JMI
          break;

        case msrPartGroup::kPartGroupSymbolBracket:
          break;

        case msrPartGroup::kPartGroupSymbolLine:
          doGenerateAWithBlock = true;
          break;

        case msrPartGroup::kPartGroupSymbolSquare:
          doGenerateAWithBlock = true;
          break;
      } // switch

      if (doGenerateAWithBlock) {
        fLilypondCodeStream <<
          endl <<
          "\\with {" <<
          endl;
      }

      gIndenter++;

      if (doGenerateAWithBlock) { // JMI
      /* ??? JMI
        if (partGroupName.size ()) {
          fLilypondCodeStream <<
            "instrumentName = " <<
            nameAsLilypondString (partGroupName) <<
            endl;
        }
        if (partGroupAbbreviation.size ()) {
          fLilypondCodeStream <<
            "shortInstrumentName = " <<
            nameAsLilypondString (partGroupAbbreviation) <<
            endl;
        }
        */
      }

      switch (partGroupSymbolKind) {
        case msrPartGroup::kPartGroupSymbolNone:
          break;

        case msrPartGroup::kPartGroupSymbolBrace: // JMI
          /*
           *
           * check whether individual part have instrument names JMI
           *
            if (partGroupInstrumentName.size ()) {
              fLilypondCodeStream << = "\\new PianoStaff";
            }
            else {
              fLilypondCodeStream << = "\\new GrandStaff";
            }
              */
          break;

        case msrPartGroup::kPartGroupSymbolBracket:
          break;

        case msrPartGroup::kPartGroupSymbolLine:
          fLilypondCodeStream <<
            "systemStartDelimiter = #'SystemStartBar" <<
            endl;
          break;

        case msrPartGroup::kPartGroupSymbolSquare:
          fLilypondCodeStream <<
            "systemStartDelimiter = #'SystemStartSquare" <<
            endl;
          break;
      } // switch

      gIndenter--;

      // generate the '\with' block ending
      // if the part group is not implicit
      if (doGenerateAWithBlock) {
        fLilypondCodeStream <<
          "}" <<
          endl;
      }

      if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
        fLilypondCodeStream << left <<
          setw (commentFieldWidth) <<
          " <<" << "% part group " <<
          partGroup->getPartGroupCombinedNameWithoutEndOfLines ();
      }
      else {
        fLilypondCodeStream <<
          " <<";
      }

      fLilypondCodeStream << endl;
      break;
  } // switch

  if (partGroupInstrumentName.size ()) { // JMI
    fLilypondCodeStream <<
      "instrumentName = \"" <<
      partGroupInstrumentName <<
      "\"" <<
      endl;
  }

  if (gGlobalLpsr2lilypondOahGroup->getConnectArpeggios ()) {
    fLilypondCodeStream <<
      "\\set PianoStaff.connectArpeggios = ##t" <<
      endl;
  }

  fLilypondCodeStream << endl;

  if (elt->getPartGroupBlockElements ().size () > 1) {
    gIndenter++; //  // decremented in visitEnd (S_lpsrPartGroupBlock& elt)
  }
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrPartGroupBlock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrPartGroupBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // fetch part group
  S_msrPartGroup
    partGroup =
      elt->getPartGroup ();

  if (elt->getPartGroupBlockElements ().size () > 1) {
    gIndenter--; // incremented in visitEnd (S_lpsrPartGroupBlock& elt)
  }

  switch (partGroup->getPartGroupImplicitKind ()) {
    case msrPartGroup::kPartGroupImplicitYes:
      // don't generate code for an implicit top-most part group block
      break;

    case msrPartGroup::kPartGroupImplicitNo:
      if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
        fLilypondCodeStream << left <<
          setw (commentFieldWidth) << ">>" <<
          "% part group " <<
          partGroup->getPartGroupCombinedNameWithoutEndOfLines ();
      }
      else {
        fLilypondCodeStream <<
          ">>";
      }

      fLilypondCodeStream << endl;

      if (fPartGroupBlocksCounter != fNumberOfPartGroupBlocks) {
        fLilypondCodeStream << endl;
      }
      break;
  } // switch
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrPartBlock& elt)
{
  // fetch part block's part
  S_msrPart
    part =
      elt->getPart ();

#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrPartBlock for '" <<
      part->asShortString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fPartGroupBlockElementsCounter++;

  fNumberOfStaffBlocksElements =
    elt->getPartBlockElementsList ().size ();

  if (part->getPartStavesMap ().size () > 1) {
    // don't generate code for a part with only one stave

    string
      partName =
        part->getPartName (),
      partAbbreviation =
        part->getPartAbbreviation ();
        /*
    string
      partInstrumentName =  // JMI
        part->getPartInstrumentName (),
      partInstrumentAbbreviation =  // JMI
        part->getPartInstrumentAbbreviation ();
        */

    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
      fLilypondCodeStream << left <<
        setw (commentFieldWidth) <<
        "\\new PianoStaff" <<
        " % part " << part->getPartCombinedName ();
    }
    else {
      fLilypondCodeStream <<
        "\\new PianoStaff";
    }
    fLilypondCodeStream << endl;

    // generate the 'with' block beginning
    fLilypondCodeStream <<
      "\\with {" <<
      endl;

    gIndenter++;

/* JMI
    if (partName.size ()) {
      fLilypondCodeStream <<
        "instrumentName = \"" <<
        partName <<
        "\"" <<
        endl;
    }
    if (partAbbreviation.size ()) {
      fLilypondCodeStream <<
        "shortInstrumentName = " <<
       nameAsLilypondString (partAbbreviation) <<
        endl;
    }
*/

    if (gGlobalLpsr2lilypondOahGroup->getConnectArpeggios ()) {
      fLilypondCodeStream <<
        "connectArpeggios = ##t" <<
        endl;
    }

    gIndenter--;

    // generate the 'with' block ending
    fLilypondCodeStream <<
      "}" <<
      endl;

    fLilypondCodeStream <<
      " <<" <<
      endl;
  }
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrPartBlock& elt)
{
  // fetch current part block's part
  S_msrPart
    part =
      elt->getPart ();

#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrPartBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (part->getPartStavesMap ().size () > 1) {
    // don't generate code for a part with only one stave
    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
      fLilypondCodeStream << left <<
        setw (commentFieldWidth) << ">>" <<
        "% part " <<
        part->getPartCombinedName ();
    }
    else {
      fLilypondCodeStream <<
        ">>";
    }

    fLilypondCodeStream << endl;

    if (fPartGroupBlockElementsCounter != fNumberOfPartGroupBlockElements) {
      fLilypondCodeStream << endl;
    }
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrStaffBlock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrStaffBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fStaffBlocksCounter++;

  S_msrStaff
    staff =
      elt->getStaff ();

  // generate the staff context
  switch (staff->getStaffKind ()) {
    case msrStaff::kStaffRegular:
      if (gGlobalLpsr2lilypondOahGroup->getJianpu ()) {
        fLilypondCodeStream << "\\new JianpuStaff";
      }
      else {
        fLilypondCodeStream << "\\new Staff";
      }
      break;

    case msrStaff::kStaffTablature:
      fLilypondCodeStream << "\\new TabStaff";
      break;

    case msrStaff::kStaffHarmony:
      fLilypondCodeStream << "\\new kStaffHarmony???";
      break;

    case msrStaff::kStaffFiguredBass:
      fLilypondCodeStream << "\\new FiguredBassStaff???";
      break;

    case msrStaff::kStaffDrum:
      fLilypondCodeStream << "\\new DrumStaff";
      break;

    case msrStaff::kStaffRythmic:
      fLilypondCodeStream << "\\new RhythmicStaff";
      break;
    } // switch

  fLilypondCodeStream <<
    " = \"" <<
    staff->getStaffName () <<
    "\"";

  fLilypondCodeStream << endl;

  // generate the 'with' block beginning
  fLilypondCodeStream <<
    "\\with {" <<
    endl;

  gIndenter++;

  // fetch part upLink
  S_msrPart
    staffPartUpLink =
      staff->getStaffPartUpLink ();

  // don't generate instrument names in the staves
  // if the containing part contains several of them
  if (staffPartUpLink->getPartStavesMap ().size () == 1) {
    // get the part upLink name to be used
    string partName =
      staffPartUpLink->
        getPartNameDisplayText ();

    if (partName.size () == 0) {
      partName =
        staffPartUpLink->
          getPartName ();
    }

    // generate the instrument name
    //* JMI BLARKBLARK
    if (partName.size ()) {
    /* JMI
      fLilypondCodeStream <<
        "instrumentName = ";

      // does the name contain hexadecimal end of lines?
      std::size_t found =
    // JMI    partName.find ("&#xd");
        partName.find ("\n");

      if (found == string::npos) {
        // no, escape quotes if any and generate the result
        fLilypondCodeStream <<
          "\"" <<
          escapeDoubleQuotes (partName) <<
          "\"" <<
          endl;
      }

      else {
        // yes, split the name into a chunks list
        // and generate a \markup{} // JMI ???
        fLilypondCodeStream <<
          endl <<
          generateMultilineMarkup (
            partName,
            markupColumnKindLeftACentered) << // JMI ???
          endl;
      }
      */
    }
    /* ??? JMI
    if (partAbbreviation.size ()) {
      fLilypondCodeStream <<
        "shortInstrumentName = " <<
        nameAsLilypondString (partAbbreviation) <<
        endl;
    }
    */

    // get the part upLink abbreviation display text to be used
    string partAbbreviation =
      staffPartUpLink->
        getPartAbbreviationDisplayText ();

    if (partAbbreviation.size () == 0) {
      partAbbreviation =
        staffPartUpLink->
          getPartAbbreviation ();
    }

/* JMI ???
    if (partAbbreviation.size ()) {
      fLilypondCodeStream <<
        "shortInstrumentName = ";

      // does the name contain hexadecimal end of lines?
      std::size_t found =
        partAbbreviation.find ("&#xd");

      if (found == string::npos) {
        // no, merely generate the name
        fLilypondCodeStream <<
          nameAsLilypondString (partAbbreviation) <<
          endl;
      }

      else {
        // yes, split the name into a chunks list
        // and generate a \markup{} // JMI ???
        fLilypondCodeStream <<
          endl <<
          generateMultilineMarkup (partAbbreviation) <<
          endl;
      }
    }
    */
  }

  gIndenter--;

  // generate the string tunings if any
  S_msrStaffDetails
    currentStaffDetails =
      staff->getCurrentStaffStaffDetails ();

  if (currentStaffDetails) {
    const list<S_msrStaffTuning>&
      staffTuningsList =
        currentStaffDetails->getStaffTuningsList ();

    if (staffTuningsList.size ()) {
      fLilypondCodeStream <<
  // JMI      "restrainOpenStrings = ##t" <<
  // JMI      endl <<
        "stringTunings = \\stringTuning <";

      list<S_msrStaffTuning>::const_iterator
        iBegin = staffTuningsList.begin (),
        iEnd   = staffTuningsList.end (),
        i      = iBegin;

      gIndenter++;

      for ( ; ; ) {
        S_msrStaffTuning
          staffTuning = (*i);

        fLilypondCodeStream <<
          msrQuarterTonesPitchKindAsString (
            gGlobalLpsrOahGroup->
              getLpsrQuarterTonesPitchesLanguageKind (),
            staffTuning->
              getStaffTuningQuarterTonesPitchKind ()) <<
          absoluteOctaveAsLilypondString (
            staffTuning->getStaffTuningOctave ());

        if (++i == iEnd) break;

        fLilypondCodeStream << ' ';
      } // for

      fLilypondCodeStream <<
        ">" <<
        endl;

      gIndenter--;

      // should letters be used for frets?
      switch (currentStaffDetails->getShowFretsKind ()) {
        case msrStaffDetails::kShowFretsNumbers:
          break;
        case msrStaffDetails::kShowFretsLetters:
          fLilypondCodeStream <<
            "tablatureFormat = #fret-letter-tablature-format" <<
            endl;
          break;
      } // switch
    }
  }

  // generate ledger lines coloring code if needed
  if (fVisitedLpsrScore->getColoredLedgerLinesIsNeeded ()) {
    fLilypondCodeStream <<
      gIndenter.indentMultiLineString (
R"(  \override LedgerLineSpanner.stencil = #MyLedgerLineSpannerPrint
  \override LedgerLineSpanner.after-line-breaking = #grob::display-objects)") <<
      endl;
  }

  // generate merge rests if needed
  if (fVisitedLpsrScore->getMergeRestsIsNeeded ()) {
    fLilypondCodeStream <<
      gTab << "\\override RestCollision.positioning-done = #merge-rests-on-positioning" <<
      endl;
  }

  // generate the 'with' block ending
  fLilypondCodeStream <<
    "}" <<
    endl;

  // generate the comment if relevant
  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
        setw (commentFieldWidth) <<
        " <<" <<
        " % staff \"" << staff->getStaffName () << "\"";
  }
  else {
    fLilypondCodeStream <<
      " <<";
  }

  fLilypondCodeStream << endl;

  if (gGlobalLpsr2lilypondOahGroup->getJianpu ()) {
    fLilypondCodeStream <<
      " \\jianpuMusic" <<
      endl;
  }

  gIndenter++; // decremented in visitEnd (S_lpsrStaffBlock& elt)
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrStaffBlock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrStaffBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  gIndenter--; // incremented in visitStart (S_lpsrStaffBlock& elt)

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
      setw (commentFieldWidth) << ">>" <<
      "% staff " <<
      elt->getStaff ()->getStaffName ();
  }
  else {
    fLilypondCodeStream <<
      ">>";
  }

  fLilypondCodeStream << endl;

  if (fStaffBlocksCounter != fNumberOfStaffBlocksElements) {
    fLilypondCodeStream << endl;
  }
}

/*
//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrNewStaffgroupBlock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrNewStaffgroupBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

   fLilypondCodeStream <<
     "\\new StaffGroup" << ' ' << "{" <<
      endl;

  gIndenter++; // decremented in visitEnd (S_lpsrStaffBlock& elt)
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrNewStaffgroupBlock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrNewStaffgroupBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  gIndenter--; // incremented in visitStart (S_lpsrStaffBlock& elt)

  fLilypondCodeStream <<
    " }" <<
    endl << endl;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrNewStaffBlock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrNewStaffBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  gIndenter++; // decremented in visitEnd (S_lpsrNewStaffBlock& elt)
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrNewStaffBlock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrNewStaffBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  gIndenter--; // incremented in visitStart (S_lpsrNewStaffBlock& elt)
}
*/

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrUseVoiceCommand& elt) // JMI ???
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrUseVoiceCommand" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  S_msrVoice
    voice = elt->getVoice ();

  S_msrStaff
    staff = voice-> getVoiceStaffUpLink ();

  msrStaff::msrStaffKind
    staffKind = staff->getStaffKind ();

  string staffContextName;
  string voiceContextName;

  switch (staffKind) {
    case msrStaff::kStaffRegular:
      staffContextName = "Staff";
      voiceContextName = "Voice";
      break;

    case msrStaff::kStaffTablature:
      staffContextName = "TabStaff";
      voiceContextName = "TabVoice";
      break;

    case msrStaff::kStaffHarmony:
      staffContextName = "ChordNames2"; // JMI
      voiceContextName = "???"; // JMI
      break;

    case msrStaff::kStaffFiguredBass:
      staffContextName = "FiguredBass";
      voiceContextName = "???"; // JMI
      break;

    case msrStaff::kStaffDrum:
      staffContextName = "DrumStaff";
      voiceContextName = "DrumVoice";
        // the "DrumVoice" alias exists, let's use it
      break;

    case msrStaff::kStaffRythmic:
      staffContextName = "RhythmicStaff";
      voiceContextName = "Voice";
        // no "RhythmicVoice" alias exists
      break;
  } // switch

 // if (voice->getStaffRelativeVoiceNumber () > 0) { JMI
    fLilypondCodeStream <<
      "\\context " << voiceContextName << " = " "\"" <<
      voice->getVoiceName () << "\"" << " <<" <<
       endl;

    gIndenter++;

    if (gGlobalLpsr2lilypondOahGroup->getNoAutoBeaming ()) {
      fLilypondCodeStream <<
        "\\set " << staffContextName << ".autoBeaming = ##f" <<
        endl;
    }

    // should a voice indication be generated?
    switch (staffKind) {
      case msrStaff::kStaffRegular:
        {
          int staffRegularVoicesCounter =
            staff->getStaffRegularVoicesCounter ();

          if (staffRegularVoicesCounter > 1) {
            switch (voice->getRegularVoiceStaffSequentialNumber ()) {
              case 1:
                fLilypondCodeStream << "\\voiceOne ";
                break;
              case 2:
                fLilypondCodeStream << "\\voiceTwo ";
                break;
              case 3:
                fLilypondCodeStream << "\\voiceThree ";
                break;
              case 4:
                fLilypondCodeStream << "\\voiceFour ";
                break;
              default:
                {}
            } // switch

            fLilypondCodeStream <<
              "% out of " <<
              staffRegularVoicesCounter <<
              " regular voices" <<
              endl;
          }
        }
      break;

      default:
        ;
    } // switch

    // fetch the part and part name
    S_msrPart
      part = staff-> getStaffPartUpLink ();
    string partName =
      part->getPartName ();

    // should a transposition be generated?
#ifdef TRACING_IS_ENABLED
    if (gGlobalTraceOahGroup->getTraceTranspositions ()) {
      gLogStream <<
        endl <<
        "Considering the generation a a voice transposition for part \"" <<
        partName <<
        "\"" <<
        endl;
    }
#endif

    // should we transpose fCurrentPart?
    bool doTransposeCurrentPart = false;
    S_msrSemiTonesPitchAndOctave
      semiTonesPitchAndOctave;

    if (partName.size ()) { // a part name is not mandatory in MusicXML
      // check by name
      if (gGlobalLpsr2lilypondOahGroup->getPartNamesTranspositionMap ().size ()) {
        map<string, S_msrSemiTonesPitchAndOctave>::const_iterator
          it =
            gGlobalLpsr2lilypondOahGroup->getPartNamesTranspositionMap ().find (
              partName);

        if (it != gGlobalLpsr2lilypondOahGroup->getPartNamesTranspositionMap ().end ()) {
          // partName is present in the map,
          // fetch the semitones pitch and octave
          semiTonesPitchAndOctave =
            (*it).second;
          doTransposeCurrentPart = true;
        }
      }
    }

    // check by ID JMI
    string partID =
      part->getPartID ();

    if (gGlobalLpsr2lilypondOahGroup->getPartIDsTranspositionMap ().size ()) {
      map<string, S_msrSemiTonesPitchAndOctave>::const_iterator
        it =
          gGlobalLpsr2lilypondOahGroup->getPartIDsTranspositionMap ().find (
            partID);

      if (it != gGlobalLpsr2lilypondOahGroup->getPartIDsTranspositionMap ().end ()) {
        // partID is present in the map,
        // fetch the semitones pitch and octave
        semiTonesPitchAndOctave =
          (*it).second;
        doTransposeCurrentPart = true;
      }
    }

    if (doTransposeCurrentPart) {
      // generate the transposition
#ifdef TRACING_IS_ENABLED
      if (gGlobalTraceOahGroup->getTraceTranspositions ()) {
        gLogStream <<
          endl <<
          "Generating a voice transposition for " <<
          semiTonesPitchAndOctave->asString () <<
          " in part \"" <<
          partName <<
          "\"" <<
          ", part ID \"" <<
          partID <<
          "\"" <<
          endl;
      }
#endif

      // fetch the LilyPond pitch and octave
      string
        semiTonesPitchAndOctaveAsLilypondString =
          msrSemiTonesPitchAndOctaveAsLilypondString (
            gGlobalLpsrOahGroup->getLpsrQuarterTonesPitchesLanguageKind (),
            semiTonesPitchAndOctave);

      fLilypondCodeStream <<
         "\\transposition " <<
         semiTonesPitchAndOctaveAsLilypondString <<
         endl <<
         "\\transpose " <<
         semiTonesPitchAndOctaveAsLilypondString << " " << "c'" <<
        endl;
    }

    // generate voice name
    fLilypondCodeStream <<
      "\\" << voice->getVoiceName () << endl;

    gIndenter--;

    fLilypondCodeStream <<
      ">>" <<
      endl;
 // } JMI
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrUseVoiceCommand& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrUseVoiceCommand" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrNewLyricsBlock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrNewLyricsBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (! gGlobalLpsr2lilypondOahGroup->getNoLilypondLyrics ()) {
    S_msrStanza stanza = elt->getStanza ();

    fLilypondCodeStream <<
      "\\new Lyrics" <<
      endl;

    gIndenter++;

    fLilypondCodeStream <<
      "\\with {" <<
      endl;

    if (gGlobalLpsr2lilypondOahGroup->getAddStanzasNumbers ()) {
      fLilypondCodeStream <<
        gTab << "stanza = \"" <<
        stanza->getStanzaNumber () <<
        ".\"" <<
        endl;
    }

    fLilypondCodeStream <<
      "}" <<
      endl;

    switch (gGlobalLpsr2lilypondOahGroup->getLyricsDurationsKind ()) {
      case kLyricsDurationsImplicit:
        fLilypondCodeStream <<
          "\\lyricsto \"" << elt->getVoice ()->getVoiceName () << "\" {" <<
          "\\" << stanza->getStanzaName () <<
          "}" <<
          endl;
        break;
      case kLyricsDurationsExplicit:
        // no \lyricsto in that case
        fLilypondCodeStream <<
          "\\" << stanza->getStanzaName () <<
          endl;
        break;
    } // switch

    gIndenter--;
  }
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrNewLyricsBlock& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrNewLyricsBlock" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (! gGlobalLpsr2lilypondOahGroup->getNoLilypondLyrics ()) {
    // JMI
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrVariableUseCommand& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrVariableUseCommand" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  gIndenter++; // decremented in visitEnd (S_lpsrVariableUseCommand& elt)
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrVariableUseCommand& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrVariableUseCommand" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  gIndenter--; // incremented in visitStart (S_lpsrVariableUseCommand& elt)
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrChordNamesContext& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrChordNamesContext" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  string
    contextTypeKindAsString =
      elt->getContextTypeKindAsString (),
    contextName =
      elt->getContextName ();

  fLilypondCodeStream <<
    "\\context " << contextTypeKindAsString <<
    " = \"" << contextName << "\"" <<
    endl;

/* JMI
  if (false) { //option JMI
    fLilypondCodeStream <<
      "\\with {" <<
      endl;

    gIndenter++;

    fLilypondCodeStream <<
      "\\override Barline.bar-extent = #'(-2 . 2)" <<
      endl <<
      "\\consists \"Bar_engraver\"" <<
      endl;

    gIndenter--;

    fLilypondCodeStream <<
      "}" <<
      endl;
  }
      */

  fLilypondCodeStream <<
    "\\" << contextName <<
    endl << endl;
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrChordNamesContext& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrChordNamesContext" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrFiguredBassContext& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrFiguredBassContext" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  string
    contextTypeKindAsString =
      elt->getContextTypeKindAsString (),
    contextName =
      elt->getContextName ();

  fLilypondCodeStream <<
    "\\context " << contextTypeKindAsString <<
    " = \"" << contextName << "\"" <<
    endl;

/* JMI
  if (false) { //option JMI
    fLilypondCodeStream <<
      "\\with {" <<
      endl;

    gIndenter++;

    fLilypondCodeStream <<
      "\\override Barline.bar-extent = #'(-2 . 2)" <<
      endl <<
      "\\consists \"Bar_engraver\"" <<
      endl;

    gIndenter--;

    fLilypondCodeStream <<
      "}" <<
      endl;
  }
  */

  fLilypondCodeStream <<
    "\\" << contextName <<
    endl << endl;
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrFiguredBassContext& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrFiguredBassContext" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrBarCommand& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrBarCommand" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrBarCommand& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrBarCommand" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
/* JMI
void lpsr2lilypondTranslator::visitStart (S_lpsrMelismaCommand& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrMelismaCommand" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  switch (elt->getMelismaKind ()) {
    case lpsrMelismaCommand::kMelismaStart:
// JMI      fLilypondCodeStream << "\\melisma ";
      break;
    case lpsrMelismaCommand::kMelismaEnd:
// JMI      fLilypondCodeStream << "\\melismaEnd ";
      break;
  } // switch
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrMelismaCommand& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrMelismaCommand" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}
*/

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrScore& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrScore" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrScore& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrScore" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrScaling& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrScaling" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrScaling& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrScaling" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrPageLayout& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrPageLayout" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

/* JMI
    // page size
    S_msrLength           fPageHeight;
    S_msrLength           fPageWidth;

    // margins
    S_msrMargin           fLeftMargin;
    S_msrMargin           fRightMargin;
    S_msrMargin           fTopMargin;
    S_msrMargin           fBottomMargin;
  */

  S_msrLength
    pageHeight =
      elt->getPageHeight ();

  if (pageHeight) {
    fLilypondCodeStream <<
      pageHeight->getLengthValue () <<
      lengthUnitAsLilypondString (pageHeight->getLengthUnitKind ()) <<
      endl;
  }
}

void lpsr2lilypondTranslator::visitEnd (S_msrPageLayout& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrPageLayout" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrSystemLayout& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrSystemLayout" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrSystemLayout& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrSystemLayout" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrStaffLayout& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrStaffLayout" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrStaffLayout& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrStaffLayout" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrAppearance& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrAppearance" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrAppearance& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrAppearance" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrCredit& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrCredit" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrCredit& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrCredit" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitStart (S_msrCreditWords& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrCreditWords" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrCreditWords& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrCreditWords" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrPartGroup& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrPartGroup" <<
      elt->asShortString () <<
      ", line " << elt->getInputLineNumber () <<
      elt->getPartGroupCombinedName () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrPartGroup& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrPartGroup" <<
      elt->getPartGroupCombinedName () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrPart& elt)
{
  string
    partCombinedName =
      elt->getPartCombinedName ();

#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrPart" <<
      partCombinedName <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceParts ()) {
    gLogStream <<
      endl <<
      "<!--=== part \"" << partCombinedName << "\"" <<
      ", line " << elt->getInputLineNumber () << " ===-->" <<
      endl;
  }
#endif

  // remember current part
  fCurrentPart = elt;

  fRemainingRestMeasuresNumber = 0; // JMI
  fOnGoingRestMeasures = false; // JMI
}

void lpsr2lilypondTranslator::visitEnd (S_msrPart& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrPart" <<
      elt->getPartCombinedName () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // forget about current part
  fCurrentPart = nullptr;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrStaff& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrStaff \"" <<
      elt->getStaffName () <<
      "\"" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fOnGoingStaff = true;
}

void lpsr2lilypondTranslator::visitEnd (S_msrStaff& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrStaff \"" <<
      elt->getStaffName () <<
      "\"" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fOnGoingStaff = false;
}

void lpsr2lilypondTranslator::visitStart (S_msrStaffTuning& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "%--> Start visiting msrStaffTuning" <<
      endl;
  }
#endif

/* JMI
  list<S_msrStaffTuning>
    staffTuningsList =
      staff->getStaffTuningsList ();

  if (staffTuningsList.size ()) {
    // \set TabStaff.stringTunings = \stringTuning <c' g' d'' a''>

    fLilypondCodeStream <<
      "\\set TabStaff.stringTunings = \\stringTuning <";

    list<S_msrStaffTuning>::const_iterator
      iBegin = staffTuningsList.begin (),
      iEnd   = staffTuningsList.end (),
      i      = iBegin;

    for ( ; ; ) {
      fLilypondCodeStream <<
        msrQuarterTonesPitchAsString (
          gGlobalLpsrOahGroup->getLpsrQuarterTonesPitchesLanguage (),
 // JMI            elt->getInputLineNumber (),
          ((*i)->getStaffTuningQuarterTonesPitch ())) <<
 // JMI       char (tolower ((*i)->getStaffTuningStep ())) <<
        absoluteOctaveAsLilypondString (
          (*i)->getStaffTuningOctave ());
      if (++i == iEnd) break;
      fLilypondCodeStream << ' ';
    } // for

    fLilypondCodeStream <<
      ">" <<
      endl;
  }
 */
}

void lpsr2lilypondTranslator::visitStart (S_msrStaffDetails& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "%--> Start visiting msrStaffDetails" <<
      endl;
  }
#endif

  // fetch staff lines number
  int
    staffLinesNumber =
      elt->getStaffLinesNumber ();

  fLilypondCodeStream <<
    endl <<
    "\\stopStaff " <<
    endl <<
    "\\override Staff.StaffSymbol.line-count = " <<
    staffLinesNumber <<
    endl <<
    "\\startStaff" <<
    endl;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrVoice& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrVoice \"" <<
      elt->getVoiceName () <<
      "\"" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fCurrentVoice = elt;

  fLilypondCodeStream <<
    fCurrentVoice->getVoiceName () <<
    " = ";

  // generate the beginning of the voice definition
  switch (fCurrentVoice->getVoiceKind ()) {
    case msrVoice::kVoiceRegular:
      switch (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) {
        case kOctaveEntryRelative:
          fLilypondCodeStream <<
            "\\relative";
          if (
            gGlobalLpsr2lilypondOahGroup->getRelativeOctaveEntrySemiTonesPitchAndOctave ()
//      JMI         !=
//            gGlobalLpsr2lilypondOahGroup->getSemiTonesPitchAndOctaveDefaultValue ()
          ) {
            // option '-rel, -relative' has been used
            fLilypondCodeStream <<
              " " <<
              msrSemiTonesPitchAndOctaveAsLilypondString (
                gGlobalLpsrOahGroup->getLpsrQuarterTonesPitchesLanguageKind (),
                gGlobalLpsr2lilypondOahGroup->getRelativeOctaveEntrySemiTonesPitchAndOctave ());
          }
          break;

        case kOctaveEntryAbsolute:
          fLilypondCodeStream <<
            "\\absolute";
          break;

        case kOctaveEntryFixed:
          fLilypondCodeStream <<
            "\\fixed " <<
            msrSemiTonesPitchAndOctaveAsLilypondString (
              gGlobalLpsrOahGroup->getLpsrQuarterTonesPitchesLanguageKind (),
              gGlobalLpsr2lilypondOahGroup->getFixedOctaveEntrySemiTonesPitchAndOctave ());
          break;
      } // switch

      fLilypondCodeStream <<
        " {" <<
        endl;
      break;

    case msrVoice::kVoiceHarmony:
      fLilypondCodeStream <<
        "\\chordmode {" <<
        endl;
      break;

    case msrVoice::kVoiceFiguredBass:
      fLilypondCodeStream <<
        "\\figuremode {" <<
        endl;
      break;
  } // switch

  gIndenter++; // decremented in visitEnd (S_msrVoice& elt)

  // use a 'global' macro?
  if (gGlobalLpsr2lilypondOahGroup->getGlobal ()) {
    fLilypondCodeStream <<
      "\\global" <<
      endl << endl;
  }

  // should \displayMusic be generated?
  if (gGlobalLpsr2lilypondOahGroup->getDisplayMusic ()) {
    fLilypondCodeStream <<
      "\\displayMusic {" <<
      endl;

    gIndenter++; // decremented in visitEnd (S_msrVoice& elt)
  }

  // language
  fLilypondCodeStream <<
    "\\language \"" <<
    msrQuarterTonesPitchesLanguageKindAsString (
      gGlobalLpsrOahGroup->
        getLpsrQuarterTonesPitchesLanguageKind ()) <<
    "\"" <<
    endl;

  // chords language
  if (gGlobalLpsrOahGroup->getLpsrChordsLanguageKind () != k_IgnatzekChords) {
    fLilypondCodeStream <<
      "\\" <<
      lpsrChordsLanguageKindAsString (
        gGlobalLpsrOahGroup->
          getLpsrChordsLanguageKind ()) <<
      "Chords" <<
      endl;
  }

  // show all bar numbers?
  if (gGlobalLpsr2lilypondOahGroup->getShowAllBarNumbers ()) {
    fLilypondCodeStream <<
      "\\set Score.barNumberVisibility = #all-bar-numbers-visible" <<
      endl <<
      "\\override Score.BarNumber.break-visibility = ##(#f #t #t)" <<
      endl << endl;
  }

  // center bar number?
  if (gGlobalLpsr2lilypondOahGroup->getBoxAroundBarNumberSet ().size ()) {
    // yes, center the boxed bar number
#ifdef TRACING_IS_ENABLED
    if (gGlobalTraceOahGroup->getTraceMeasuresNumbers ()) {
      gLogStream <<
        endl <<
        "Centering boxed LilyPond measure numbers" <<
        ", line " << elt->getInputLineNumber () << " ===-->" <<
        endl;
    }
#endif

    fLilypondCodeStream <<
      "\\override Score.BarNumber.self-alignment-X = #CENTER" <<
      endl;
  }

  // compress full measure rests?
  if (gGlobalLpsr2lilypondOahGroup->getCompressFullMeasureRests ()) {
    fLilypondCodeStream <<
      "\\compressFullBarRests" <<
      endl <<
      "\\set restNumberThreshold = 0" <<
      endl << endl;
  }

// JMI   \set Score.alternativeNumberingStyle = #'numbers-with-letters

/* JMI
  if (
    fCurrentVoice->getVoiceContainsRestMeasures ()
      ||
    gGlobalLpsr2lilypondOahGroup->getCompressFullMeasureRests ()
  ) {
    fLilypondCodeStream <<
      "\\compressFullBarRests" <<
      endl;

    gInde   nter++; // JMI ???
  }
*/

  if (gGlobalLpsr2lilypondOahGroup->getAccidentalStyleKind () != kDefault) {
    fLilypondCodeStream <<
      "\\accidentalStyle Score." <<
      lpsrAccidentalStyleKindAsString (
        gGlobalLpsr2lilypondOahGroup->getAccidentalStyleKind ()) <<
      endl << endl;
  }

  // reset fCurrentVoiceClef
  fCurrentVoiceClef = nullptr;

  // reset fCurrentOctaveEntryReference if relevant
  switch (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) {
    case kOctaveEntryRelative:
      // forget about the current reference:
      // it should be set from the LilyPond preferences here
      setCurrentOctaveEntryReferenceFromTheLilypondOah ();
      break;
    case kOctaveEntryAbsolute:
      break;
    case kOctaveEntryFixed:
      break;
  } // switch

  fVoiceIsCurrentlySenzaMisura = false;

  fOnGoingVoice = true;

  switch (fCurrentVoice->getVoiceKind ()) { // JMI
    case msrVoice::kVoiceRegular:
      break;

    case msrVoice::kVoiceHarmony:
      fOnGoingHarmonyVoice = true;
      break;

    case msrVoice::kVoiceFiguredBass:
      fOnGoingFiguredBassVoice = true;
      break;
  } // switch

  // reset current voice measures counter
  fCurrentVoiceMeasuresCounter = 0; // none have been found

  // force durations to be displayed explicitly
  // at the beginning of the voice
  fLastMetWholeNotes = rational (0, 1);
}

void lpsr2lilypondTranslator::visitEnd (S_msrVoice& elt)
{
  gIndenter--; // incremented in visitStart (S_msrVoice& elt)

#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrVoice \"" <<
      elt->getVoiceName () <<
      "\"" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  /* JMI
  if (
    fCurrentVoice->getVoiceContainsRestMeasures ()
      ||
    gGlobalLpsr2lilypondOahGroup->getCompressFullMeasureRests ()
  ) {
    fLilypondCodeStream <<
  // JMI    "}" <<
      endl;

    g Inden ter--; // JMI ???
  */

  if (gGlobalLpsr2lilypondOahGroup->getDisplayMusic ()) {
    fLilypondCodeStream <<
      "}";

    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
      fLilypondCodeStream << left <<
        setw (commentFieldWidth) <<
        " % displayMusic";
    }

    fLilypondCodeStream << endl;

    gIndenter--; // incremented in visitStart (S_msrVoice& elt)
  }

  // generate the end of the voice definition
  switch (elt->getVoiceKind ()) { // JMI putzer
    case msrVoice::kVoiceRegular:
      fLilypondCodeStream <<
        "}";
      break;

    case msrVoice::kVoiceHarmony:
      fLilypondCodeStream <<
        "}";
      break;

    case msrVoice::kVoiceFiguredBass:
      fLilypondCodeStream <<
        "}";
      break;
  } // switch

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      " % voice " << elt->getVoiceName ();
  }

  fLilypondCodeStream <<
    endl << endl;

  // set relevant 'ongoing' variables
  switch (elt->getVoiceKind ()) {
    case msrVoice::kVoiceRegular:
      break;

    case msrVoice::kVoiceHarmony:
      fOnGoingHarmonyVoice = false;
      break;

    case msrVoice::kVoiceFiguredBass:
      fOnGoingFiguredBassVoice = false;
      break;
  } // switch

  fCurrentVoice = nullptr;
  fOnGoingVoice = false;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrVoiceStaffChange& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrVoiceStaffChange '" <<
      elt->asString () << "'" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    endl <<
    "\\change Staff=\"" <<
    elt->getStaffToChangeTo ()->getStaffName () <<
    "\"";

  if (
    gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()
      ||
    gGlobalLpsr2lilypondOahGroup->getGeneratePositionsInMeasures ()
  ) {
    generateInputLineNumberAndOrPositionInMeasureAsAComment (
      elt);
  }

  fLilypondCodeStream << endl;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrHarmony& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrHarmony '" <<
      elt->asString () <<
      "'" <<
      ", fOnGoingNotesStack.size () = " <<  fOnGoingNotesStack.size () <<
      ", fOnGoingChord = " << booleanAsString (fOnGoingChord) <<
      ", fOnGoingHarmonyVoice = " << booleanAsString (fOnGoingHarmonyVoice) <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (fOnGoingNotesStack.size () > 0) {
  /* JMI
#ifdef TRACING_IS_ENABLED
    if (gGlobalTraceOahGroup->getTraceHarmonies ()) {
      fLilypondCodeStream <<
        "%{ fOnGoingNotesStack.size () S_msrHarmony JMI " <<
        elt->asString () <<
        " %}" <<
        endl;
    }
#endif
*/
  }

  else if (fOnGoingChord) { // JMI
  }

  else if (fOnGoingHarmonyVoice) {
    fLilypondCodeStream <<
      harmonyAsLilypondString (elt) <<
      ' ';

    if (
      gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()
        ||
      gGlobalLpsr2lilypondOahGroup->getGeneratePositionsInMeasures ()
    ) {
      generateInputLineNumberAndOrPositionInMeasureAsAComment (
        elt);
    }
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrFrame& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrHarmony '" <<
      elt->asString () <<
      "'" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (fOnGoingNotesStack.size () > 0) {
#ifdef TRACING_IS_ENABLED
    if (gGlobalTraceOahGroup->getTraceFrames ()) {
      fLilypondCodeStream <<
        "%{ " << elt->asString () << " %}" <<
        endl;
    }
#endif

    fLilypondCodeStream <<
      frameAsLilypondString (elt) <<
      endl;
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrFiguredBass& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrFiguredBass '" <<
      elt->asString () <<
      "'" <<
      ", fOnGoingNotesStack.size () = " << fOnGoingNotesStack.size () <<
      ", fOnGoingChord = " << booleanAsString (fOnGoingChord) <<
      ", fOnGoingFiguredBassVoice = " << booleanAsString (fOnGoingFiguredBassVoice) <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fCurrentFiguredBass = elt;

  if (fOnGoingNotesStack.size () > 0) {
#ifdef TRACING_IS_ENABLED
    if (gGlobalTraceOahGroup->getTraceFiguredBasses ()) {
      fLilypondCodeStream <<
        "%{ fOnGoingNotesStack.size () S_msrFiguredBass JMI " << fCurrentFiguredBass->asString () << " %}" <<
        endl;
    }
#endif
  }

  else if (fOnGoingChord) { // JMI
  }

  else if (fOnGoingFiguredBassVoice) {
    fLilypondCodeStream <<
      figuredBassAsLilypondString (elt) <<
      ' ';

    if (
      gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()
        ||
      gGlobalLpsr2lilypondOahGroup->getGeneratePositionsInMeasures ()
    ) {
      generateInputLineNumberAndOrPositionInMeasureAsAComment (
        fCurrentFiguredBass);
    }
  }

  fCurrentFiguredBassFiguresCounter = 0;
}

void lpsr2lilypondTranslator::visitStart (S_msrFigure& elt)
{
/* JMI
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrFigure '" <<
      elt->asString () <<
      "'" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (fOnGoingFiguredBassVoice) {
    fCurrentFiguredBassFiguresCounter++;

    // is the figured bass parenthesized?
    msrFiguredBass::msrFiguredBassParenthesesKind
      figuredBassParenthesesKind =
        fCurrentFiguredBass->
          getFiguredBassParenthesesKind ();

    // generate the figure number
    switch (figuredBassParenthesesKind) {
      case msrFiguredBass::kFiguredBassParenthesesYes:
        fLilypondCodeStream << "[";
        break;
      case msrFiguredBass::kFiguredBassParenthesesNo:
        break;
    } // switch

    fLilypondCodeStream <<
      elt->getFigureNumber ();

    switch (figuredBassParenthesesKind) {
      case msrFiguredBass::kFiguredBassParenthesesYes:
        fLilypondCodeStream << "]";
        break;
      case msrFiguredBass::kFiguredBassParenthesesNo:
        break;
    } // switch

    // handle the figure prefix
    switch (elt->getFigurePrefixKind ()) {
      case msrFigure::k_NoFigurePrefix:
        break;
      case msrFigure::kDoubleFlatPrefix:
        fLilypondCodeStream << "--";
        break;
      case msrFigure::kFlatPrefix:
        fLilypondCodeStream << "-";
        break;
      case msrFigure::kFlatFlatPrefix:
        fLilypondCodeStream << "flat flat";
        break;
      case msrFigure::kNaturalPrefix:
        fLilypondCodeStream << "!";
        break;
      case msrFigure::kSharpSharpPrefix:
        fLilypondCodeStream << "sharp sharp";
        break;
      case msrFigure::kSharpPrefix:
        fLilypondCodeStream << "+";
        break;
      case msrFigure::kDoubleSharpPrefix:
        fLilypondCodeStream << "++";
        break;
    } // switch

    // handle the figure suffix
    switch (elt->getFigureSuffixKind ()) {
      case msrFigure::k_NoFigureSuffix:
        break;
      case msrFigure::kDoubleFlatSuffix:
        fLilypondCodeStream << "double flat";
        break;
      case msrFigure::kFlatSuffix:
        fLilypondCodeStream << "flat";
        break;
      case msrFigure::kFlatFlatSuffix:
        fLilypondCodeStream << "flat flat";
        break;
      case msrFigure::kNaturalSuffix:
        fLilypondCodeStream << "natural";
        break;
      case msrFigure::kSharpSharpSuffix:
        fLilypondCodeStream << "sharp sharp";
        break;
      case msrFigure::kSharpSuffix:
        fLilypondCodeStream << "sharp";
        break;
      case msrFigure::kDoubleSharpSuffix:
        fLilypondCodeStream << "souble sharp";
        break;
      case msrFigure::kSlashSuffix:
        fLilypondCodeStream << "/";
        break;
    } // switch

    // generate a space if not last figure in figured bass
    if (
      fCurrentFiguredBassFiguresCounter
        <
      fCurrentFiguredBass->getFiguredBassFiguresList ().size ()
    ) {
      fLilypondCodeStream << ' ';
    }
  }
  */
}

void lpsr2lilypondTranslator::visitEnd (S_msrFiguredBass& elt)
{
#ifdef TRACING_IS_ENABLED
  int inputLineNumber =
    elt->getInputLineNumber ();

  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrFiguredBass '" <<
      elt->asString () <<
      "'" <<
      ", line " << inputLineNumber <<
      endl;
  }
#endif

/* JMI
  if (fOnGoingFiguredBassVoice) {
    fLilypondCodeStream <<
      ">";

    // print figured bass duration
    msrTupletFactor
      figuredBassTupletFactor =
        elt->getFiguredBassTupletFactor ();

    if (figuredBassTupletFactor.isEqualToOne ()) {
      // use figured bass sounding whole notes
      fLilypondCodeStream <<
        durationAsLilypondString (
          inputLineNumber,
          elt->
            getFiguredBassSoundingWholeNotes ());
    }
    else {
      // use figured bass display whole notes and tuplet factor
      fLilypondCodeStream <<
        durationAsLilypondString (
          inputLineNumber,
          elt->
            getFiguredBassDisplayWholeNotes ()) <<
        "*" <<
        figuredBassTupletFactor.asRational ();
    }

    fLilypondCodeStream <<
      ' ';
  }
  */
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrSegment& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "%--> Start visiting msrSegment '" <<
      elt->getSegmentAbsoluteNumber () << "'" <<
      endl;
  }
#endif

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      "% start of segment " <<
      elt->getSegmentAbsoluteNumber () <<
      ", line " <<
      elt->getInputLineNumber () <<
      endl;

    gIndenter++; // decremented in visitEnd (S_msrSegment& elt)
  }
}

void lpsr2lilypondTranslator::visitEnd (S_msrSegment& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "%--> End visiting msrSegment '" <<
      elt->getSegmentAbsoluteNumber () << "'" <<
      endl;
  }
#endif

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    gIndenter--; // incremented in visitStart (S_msrSegment& elt)

    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      "% end of segment" <<
      elt->getSegmentAbsoluteNumber () <<
      ", line " <<
      elt->getInputLineNumber () <<
      endl;
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrMeasure& elt)
{
  int
    inputLineNumber =
      elt->getInputLineNumber ();

  string
    measureNumber =
      elt->getMeasureElementMeasureNumber ();

  msrMeasure::msrMeasureKind
    measureKind =
      elt->getMeasureKind ();

  int
    measurePuristNumber =
      elt->getMeasurePuristNumber ();

#ifdef TRACING_IS_ENABLED
  msrMeasure::msrMeasureEndRegularKind
    measureEndRegularKind =
      elt-> getMeasureEndRegularKind ();
#endif

#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrMeasure '" <<
      measureNumber <<
      "', " <<
      msrMeasure::measureKindAsString (measureKind) <<
      ", " <<
      msrMeasure::measureEndRegularKindAsString (
        measureEndRegularKind) <<
      ", measurePuristNumber = '" <<
      measurePuristNumber <<
      "', onGoingRestMeasures = '" <<
      booleanAsString (
        fOnGoingRestMeasures) <<
      ", line " << inputLineNumber <<
      endl;
  }
#endif

  // should we generate a box around this bar number?
  {
    set<int>::const_iterator
      it =
        gGlobalLpsr2lilypondOahGroup->
          getBoxAroundBarNumberSet ().find (measurePuristNumber);

    if (it != gGlobalLpsr2lilypondOahGroup->getBoxAroundBarNumberSet ().end ()) {
      // yes, generate a box around the bar number
  #ifdef TRACING_IS_ENABLED
      if (gGlobalTraceOahGroup->getTraceMeasuresNumbers ()) {
        gLogStream <<
          endl <<
          "Generating a box around LilyPond measure purist number '" <<
          measurePuristNumber <<
          "', line " << inputLineNumber << " ===-->" <<
          endl;
      }
  #endif

      fLilypondCodeStream <<
        "\\boxAroundNextBarNumber" <<
        endl;
    }
  }

  // should we reset the measure purist number?
  {
    map<string, int>::const_iterator
      it =
        gGlobalLpsr2lilypondOahGroup->
          getResetMeasureElementMeasureNumberMap ().find (measureNumber);

    if (it != gGlobalLpsr2lilypondOahGroup->getResetMeasureElementMeasureNumberMap ().end ()) {
      // yes, reset measure number
      int lilypondMeasureNumber = (*it).second;

      if (to_string (lilypondMeasureNumber) != measureNumber) {
  #ifdef TRACING_IS_ENABLED
        if (gGlobalTraceOahGroup->getTraceMeasuresNumbers ()) {
          gLogStream <<
            endl <<
            "Resetting LilyPond measure number from '" <<
            measureNumber <<
            "' to " <<
            lilypondMeasureNumber <<
            "', line " << inputLineNumber << " ===-->" <<
            endl;
        }
  #endif

        fLilypondCodeStream <<
          "\\set Score.currentBarNumber = #" <<
          lilypondMeasureNumber <<
          endl;
      }
      else {
  #ifdef TRACING_IS_ENABLED
        if (gGlobalTraceOahGroup->getTraceMeasuresNumbers ()) {
          gLogStream <<
            endl <<
            "Cannot reset measure LilyPond number from '" <<
            measureNumber <<
            "' to " <<
            lilypondMeasureNumber <<
            ": they're one and the same" <<
            "', line " << inputLineNumber << " ===-->" <<
            endl;
        }
  #endif
      }
    }
  }

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceMeasures ()) {
    gLogStream <<
      endl <<
      "% <!--=== measure '" << measureNumber <<
      "' start, " <<
      msrMeasure::measureEndRegularKindAsString (
        measureEndRegularKind) <<
      ", measurePuristNumber = '" <<
      measurePuristNumber <<
      "', onGoingRestMeasures = '" <<
      booleanAsString (
        fOnGoingRestMeasures) <<
      "', line " << inputLineNumber << " ===-->" <<
      endl;
  }
#endif

  // generate comment if relevant
  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      "% start of " <<
      msrMeasure::measureKindAsString (elt->getMeasureKind ()) <<
      " measure " <<
      measureNumber <<
      ", line " << inputLineNumber <<
      endl;

    gIndenter++; // decremented in visitEnd (S_msrMeasure& elt)
  }

  // take this measure into account for counting
  switch (elt->getMeasureKind ()) {
    case msrMeasure::kMeasureKindUnknown:
      fCurrentVoiceMeasuresCounter++;
      break;
    case msrMeasure::kMeasureKindRegular:
      fCurrentVoiceMeasuresCounter++;
      break;
    case msrMeasure::kMeasureKindAnacrusis:
      // keep fCurrentVoiceMeasuresCounter at 0
      break;
    case msrMeasure::kMeasureKindIncompleteStandalone:
    case msrMeasure::kMeasureKindIncompleteLastInRepeatCommonPart:
    case msrMeasure::kMeasureKindIncompleteLastInRepeatHookedEnding:
    case msrMeasure::kMeasureKindIncompleteLastInRepeatHooklessEnding:
    case msrMeasure::kMeasureKindIncompleteNextMeasureAfterCommonPart:
    case msrMeasure::kMeasureKindIncompleteNextMeasureAfterHookedEnding:
    case msrMeasure::kMeasureKindIncompleteNextMeasureAfterHooklessEnding:
      fCurrentVoiceMeasuresCounter++;
      break;
    case msrMeasure::kMeasureKindOvercomplete:
      fCurrentVoiceMeasuresCounter++;
      break;
    case msrMeasure::kMeasureKindCadenza:
      fCurrentVoiceMeasuresCounter++;
      break;
    case msrMeasure::kMeasureKindMusicallyEmpty:
      fCurrentVoiceMeasuresCounter++;
      break;
  } // switch

  // force durations to be displayed explicitly
  // for the notes at the beginning of the measure
  fLastMetWholeNotes = rational (0, 1);

  // is this the end of a cadenza?
  if (
    fOnGoingVoiceCadenza
      &&
    measureKind != msrMeasure::kMeasureKindOvercomplete
  ) {
  /* JMI
    fLilypondCodeStream <<
      endl <<
      "\\cadenzaOff" <<
 // JMI     " \\undo \\omit Staff.TimeSignature" <<
      endl <<
      "\\bar \"|\" "; // JMI ???
      */

    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
      fLilypondCodeStream <<
        " % kMeasureKindOvercomplete End";
    }

    fLilypondCodeStream <<
      endl;

    fOnGoingVoiceCadenza = false;
  }

  switch (measureKind) {
    case msrMeasure::kMeasureKindUnknown:
      {
        stringstream s;

        s <<
          "measure '" << measureNumber <<
          "' is of unknown kind in lpsr2lilypondTranslator";

if (false) // JMI
        msrInternalError (
          gGlobalOahOahGroup->getInputSourceName (),
          inputLineNumber,
          __FILE__, __LINE__,
          s.str ());
else
        msrInternalWarning (
          gGlobalOahOahGroup->getInputSourceName (),
          inputLineNumber,
          s.str ());
      }
      break;

    case msrMeasure::kMeasureKindRegular:
      break;

    case msrMeasure::kMeasureKindAnacrusis:
      if (elt->getMeasureFirstInVoice ()) {
        // only generate '\partial' at the beginning of a voice
        string
          upbeatDuration =
            wholeNotesAsLilypondString (
              inputLineNumber,
              elt->getCurrentMeasureWholeNotesDuration ());

        fLilypondCodeStream <<
          "\\partial " << upbeatDuration <<
          endl;
      }
      break;

    case msrMeasure::kMeasureKindIncompleteStandalone:
    case msrMeasure::kMeasureKindIncompleteLastInRepeatCommonPart:
    case msrMeasure::kMeasureKindIncompleteLastInRepeatHookedEnding:
    case msrMeasure::kMeasureKindIncompleteLastInRepeatHooklessEnding:
    case msrMeasure::kMeasureKindIncompleteNextMeasureAfterCommonPart:
    case msrMeasure::kMeasureKindIncompleteNextMeasureAfterHookedEnding:
    case msrMeasure::kMeasureKindIncompleteNextMeasureAfterHooklessEnding:
      {
        rational
          currentMeasureWholeNotesDuration =
            elt->getCurrentMeasureWholeNotesDuration ();

        rational
          fullMeasureWholeNotesDuration =
            elt->getFullMeasureWholeNotesDuration ();

        // we should set the score current measure whole notes in this case
        rational
          ratioToFullMeasureWholeNotesDuration =
            currentMeasureWholeNotesDuration / fullMeasureWholeNotesDuration;
        ratioToFullMeasureWholeNotesDuration.rationalise ();

#ifdef TRACING_IS_ENABLED
        if (gGlobalTraceOahGroup->getTraceMeasuresDetails ()) {
          const int fieldWidth = 27;

          fLilypondCodeStream << left <<
            "% Setting the current measure whole notes for measure " <<
            setw (fieldWidth) <<
            measureNumber <<
            ", line = " << inputLineNumber <<
            endl <<
            setw (fieldWidth) <<
            "% currentMeasureWholeNotesDuration" << " = " <<
            currentMeasureWholeNotesDuration <<
            endl <<
            setw (fieldWidth) <<
            "% fullMeasureWholeNotesDuration" << " = " <<
            fullMeasureWholeNotesDuration <<
            endl <<
            setw (fieldWidth) <<
            "% ratioToFullMeasureWholeNotesDuration" << " = " <<
            ratioToFullMeasureWholeNotesDuration <<
            endl << endl;
        }
#endif

        if (ratioToFullMeasureWholeNotesDuration == rational (1, 1)) {
          stringstream s;

          s <<
            "underfull measure '" << measureNumber <<
            "' has actually the full measure whole notes";

     // JMI       msrInternalError (
          msrInternalWarning (
            gGlobalOahOahGroup->getInputSourceName (),
            inputLineNumber,
    // JMI        __FILE__, __LINE__,
            s.str ());
        }

        else {
          /* JMI
          fLilypondCodeStream <<
            "\\set Score.measureLength = #(ly:make-moment " <<
            currentMeasureWholeNotesDuration.toString () <<
            ")" <<
            endl;
    */

          // should we generate a line break?
          if (gGlobalLpsr2lilypondOahGroup->getBreakLinesAtIncompleteRightMeasures ()) {
            fLilypondCodeStream <<
              "\\break" <<
              endl;
          }
        }
      }
      break;

    case msrMeasure::kMeasureKindOvercomplete:
      if (! fOnGoingVoiceCadenza) {
        fLilypondCodeStream <<
          endl <<
          "\\cadenzaOn" <<
          " \\omit Staff.TimeSignature";

        if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
          fLilypondCodeStream << " % kMeasureKindOvercomplete Start";
        }

        fLilypondCodeStream << endl;

        fOnGoingVoiceCadenza = true;
      }
      break;

    case msrMeasure::kMeasureKindCadenza:
      if (! fOnGoingVoiceCadenza) {
        fLilypondCodeStream <<
          endl <<
          "\\cadenzaOn";

        if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
          fLilypondCodeStream << " % kMeasureKindCadenza Start";
        }

        fLilypondCodeStream << endl;

        fLilypondCodeStream <<
          "\\once\\omit Staff.TimeSignature" <<
          endl;

        fOnGoingVoiceCadenza = true;
      }
      break;

    case msrMeasure::kMeasureKindMusicallyEmpty:
      {
        // fetch measure's voice
        S_msrVoice
          measureVoice =
            elt->
              fetchMeasureVoiceUpLink ();

        // fetch measure's part
        S_msrPart
          measurePart =
            measureVoice->
              getVoiceStaffUpLink ()->
                getStaffPartUpLink ();

        rational
          measureWholeNotesDuration =
            measurePart->
              getPartMeasuresWholeNotesDurationsVector () [
                 elt->getMeasureOrdinalNumberInVoice () - 1 ];

        // generate the skip name
        // take voice kind into account may be useful for debug
        switch (measureVoice->getVoiceKind ()) {
          case msrVoice::kVoiceRegular:
            fLilypondCodeStream <<
              "s%{21%}";
            break;

          case msrVoice::kVoiceHarmony:
          case msrVoice::kVoiceFiguredBass:
            fLilypondCodeStream <<
              "s%{21%}";
            break;
        } // switch

        // generate the duration of the skip
        fLilypondCodeStream <<
          wholeNotesAsLilypondString (
            inputLineNumber,
            measureWholeNotesDuration);

        // generate a bar check if this makes the measure full
        if (
          measureWholeNotesDuration
            ==
          elt->getFullMeasureWholeNotesDuration ()
        ) {
          fLilypondCodeStream << " | ";
        }
      }
      break;
  } // switch
}

void lpsr2lilypondTranslator::visitEnd (S_msrMeasure& elt)
{
  int
    inputLineNumber =
      elt->getInputLineNumber ();

  string
    measureNumber =
      elt->getMeasureElementMeasureNumber ();

  msrMeasure::msrMeasureKind
    measureKind =
      elt->getMeasureKind ();

  int
    measurePuristNumber =
      elt->getMeasurePuristNumber ();

#ifdef TRACING_IS_ENABLED
  msrMeasure::msrMeasureEndRegularKind
    measureEndRegularKind =
      elt-> getMeasureEndRegularKind ();
#endif

#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrMeasure '" <<
      measureNumber <<
      "', " <<
      msrMeasure::measureKindAsString (measureKind) <<
      ", " <<
      msrMeasure::measureEndRegularKindAsString (
        measureEndRegularKind) <<
      ", measurePuristNumber = '" <<
      measurePuristNumber <<
      "', onGoingRestMeasures = '" <<
      booleanAsString (
        fOnGoingRestMeasures) <<
      ", line " << inputLineNumber <<
      endl;
  }
#endif

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceMeasures ()) {
    gLogStream <<
      endl <<
      "% <!--=== measure '" << measureNumber <<
      "' end, " <<
      msrMeasure::measureEndRegularKindAsString (
        measureEndRegularKind) <<
     "' end, measurePuristNumber = '" << measurePuristNumber << "'" <<
      ", onGoingRestMeasures = '" <<
      booleanAsString (
        fOnGoingRestMeasures) <<
      "', line " << inputLineNumber << " ===-->" <<
      endl;
  }
#endif

  if (! fOnGoingRestMeasures) {
    // handle the measure
    switch (measureKind) {
      case msrMeasure::kMeasureKindUnknown: // should not occur
        fLilypondCodeStream <<
          "%{ measureKindUnknown, " <<
          measurePuristNumber + 1 <<
          " %}" <<
          endl;
        break;

      case msrMeasure::kMeasureKindRegular:
        {
        }
        break;

      case msrMeasure::kMeasureKindAnacrusis:
        break;

      case msrMeasure::kMeasureKindIncompleteStandalone:
        break;

      case msrMeasure::kMeasureKindIncompleteLastInRepeatCommonPart:
      case msrMeasure::kMeasureKindIncompleteLastInRepeatHookedEnding:
      case msrMeasure::kMeasureKindIncompleteLastInRepeatHooklessEnding:
      case msrMeasure::kMeasureKindIncompleteNextMeasureAfterCommonPart:
      case msrMeasure::kMeasureKindIncompleteNextMeasureAfterHookedEnding:
      case msrMeasure::kMeasureKindIncompleteNextMeasureAfterHooklessEnding:
        break;

      case msrMeasure::kMeasureKindOvercomplete:
        fLilypondCodeStream <<
          endl <<
          "\\cadenzaOff" <<
          " \\undo \\omit Staff.TimeSignature |" <<
          endl;

        fOnGoingVoiceCadenza = false;
        break;

      case msrMeasure::kMeasureKindCadenza:
        fLilypondCodeStream <<
          endl <<
          "\\cadenzaOff" <<
          endl <<
          "\\bar \"|\"" << // JMI ???
          endl;

        fOnGoingVoiceCadenza = false;
        break;

      case msrMeasure::kMeasureKindMusicallyEmpty:
        if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
          fLilypondCodeStream <<
            "%{ emptyMeasureKind" <<
            ", line " << inputLineNumber <<
            " %} % " <<
            measurePuristNumber + 1 <<
            endl;
        }
        break;
    } // switch

    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
      gIndenter--; // incremented in visitStart (S_msrMeasure& elt)

      fLilypondCodeStream << left <<
        setw (commentFieldWidth) <<
        "% end of " <<
        msrMeasure::measureKindAsString (elt->getMeasureKind ()) <<
        " measure " <<
        measureNumber <<
        ", line " << inputLineNumber <<
        endl << endl;
    }

    if (gGlobalLpsr2lilypondOahGroup->getSeparatorLineEveryNMeasures () > 0) {
      if (
        fCurrentVoiceMeasuresCounter
          %
        gGlobalLpsr2lilypondOahGroup->getSeparatorLineEveryNMeasures ()
          ==
        0)
        fLilypondCodeStream <<
          endl <<
          "% ============================= " <<
          endl << endl;
    }
  }

  // should we generate a line break?
  {
    set<string>::const_iterator
      it =
        gGlobalLpsr2lilypondOahGroup->
          getBreakLineAfterMeasureNumberSet ().find (measureNumber);

    if (it != gGlobalLpsr2lilypondOahGroup->getBreakLineAfterMeasureNumberSet ().end ()) {
      // yes, generate a line break command
  #ifdef TRACING_IS_ENABLED
        if (gGlobalTraceOahGroup->getTraceLineBreaks ()) {
          gLogStream <<
            endl <<
            "Adding a LilyPond line break after measure number '" <<
            measureNumber <<
            "', line " << inputLineNumber << " ===-->" <<
            endl;
        }
  #endif

        fLilypondCodeStream <<
          "\\break" <<
          endl;
    }

  /* JMI
    else {
  #ifdef TRACING_IS_ENABLED
     if (gGlobalTraceOahGroup->getTraceLineBreaks ()) { // JMI
        gLogStream <<
          endl <<
          "Measure number '" <<
          measureNumber <<
          "' not found in gGlobalLpsr2lilypondOahGroup->getBreakLineAfterMeasureNumberSet ()" <<
          ", line " << inputLineNumber <<
          endl;
      }
  #endif
    }
    */
  }

  // should we generate a page break?
  {
    set<string>::const_iterator
      it =
        gGlobalLpsr2lilypondOahGroup->
          getBreakPageAfterMeasureNumberSet ().find (measureNumber);

    if (it != gGlobalLpsr2lilypondOahGroup->getBreakPageAfterMeasureNumberSet ().end ()) {
      // yes, generate a page break command
  #ifdef TRACING_IS_ENABLED
        if (gGlobalTraceOahGroup->getTracePageBreaks ()) {
          gLogStream <<
            endl <<
            "Adding a LilyPond page break after measure number '" <<
            measureNumber <<
            "', line " << inputLineNumber << " ===-->" <<
            endl;
        }
  #endif

        fLilypondCodeStream <<
          "\\pageBreak" <<
          endl;
    }

  /* JMI
    else {
  #ifdef TRACING_IS_ENABLED
     if (gGlobalTraceOahGroup->getTracePageBreaks ()) { // JMI
        gLogStream <<
          endl <<
          "Measure number '" <<
          measureNumber <<
          "' not found in gGlobalLpsr2lilypondOahGroup->getBreakPageAfterMeasureNumberSet ()" <<
          ", line " << inputLineNumber <<
          endl;
      }
  #endif
    }
 */
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrStanza& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrStanza \"" <<
      elt->getStanzaName () <<
      "\"" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (! gGlobalLpsr2lilypondOahGroup->getNoLilypondLyrics ()) {
    // don't generate code for the stanza inside the code for the voice
    fGenerateCodeForOngoingNonEmptyStanza =
      ! fOnGoingVoice
        &&
      elt->getStanzaTextPresent ();

    if (fGenerateCodeForOngoingNonEmptyStanza) {
      fLilypondCodeStream <<
        elt->getStanzaName () << " = " << "\\lyricmode {" <<
        endl;

      gIndenter++; // decremented in visitEnd (S_msrStanza& elt)

      fLilypondCodeStream <<
        // set associatedVoice so that
        // both double hyphens and double underscores can be used
        // to draw hyphenated lines and extenders under melismata correctly
        "\\set associatedVoice = #\"" <<
        elt->
          getStanzaVoiceUpLink ()->
            getVoiceName () <<
        "\"" <<
        endl <<
        "\\set ignoreMelismata = ##t" <<
        endl;
    }
  }
}

void lpsr2lilypondTranslator::visitEnd (S_msrStanza& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrStanza \"" <<
      elt->getStanzaName () <<
      "\"" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (! gGlobalLpsr2lilypondOahGroup->getNoLilypondLyrics ()) {
    if (fGenerateCodeForOngoingNonEmptyStanza) {
      gIndenter--; // incremented in visitStart (S_msrStanza& elt)

      fLilypondCodeStream <<
        endl <<
        "}" <<
        endl << endl;
    }

    fGenerateCodeForOngoingNonEmptyStanza = false;
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrSyllable& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrSyllable '" <<
      elt->asString () <<
      "'" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (! gGlobalLpsr2lilypondOahGroup->getNoLilypondLyrics ()) {
    if (fGenerateCodeForOngoingNonEmptyStanza) {
      switch (elt->getSyllableKind ()) {
        case msrSyllable::kSyllableNone: // JMI
          break;

        case msrSyllable::kSyllableSingle:
          writeTextsListAsLilypondString (
            elt->getSyllableTextsList (),
            fLilypondCodeStream);

          switch (gGlobalLpsr2lilypondOahGroup->getLyricsDurationsKind ()) {
            case kLyricsDurationsImplicit:
              // don't generate a duration for automatic lyrics durations
              break;
            case kLyricsDurationsExplicit:
              fLilypondCodeStream <<
                elt->syllableWholeNotesAsMsrString ();
              break;
          } // switch

          fLilypondCodeStream <<
            ' ';

#ifdef TRACING_IS_ENABLED
          if (gGlobalTraceOahGroup->getTraceLyrics ()) {
            fLilypondCodeStream <<
              "%{ syllableSingle %} ";
          }
#endif
          break;

        case msrSyllable::kSyllableBegin:
          writeTextsListAsLilypondString (
            elt->getSyllableTextsList (),
            fLilypondCodeStream);

          switch (gGlobalLpsr2lilypondOahGroup->getLyricsDurationsKind ()) {
            case kLyricsDurationsImplicit:
              // don't generate a duration for automatic lyrics durations
              break;
            case kLyricsDurationsExplicit:
              fLilypondCodeStream <<
                elt->syllableWholeNotesAsMsrString ();
              break;
          } // switch

          fLilypondCodeStream <<
            " -- ";

#ifdef TRACING_IS_ENABLED
          if (gGlobalTraceOahGroup->getTraceLyrics ()) {
            fLilypondCodeStream <<
              "%{ syllableBegin %} ";
          }
#endif
          break;

        case msrSyllable::kSyllableMiddle:
          writeTextsListAsLilypondString (
            elt->getSyllableTextsList (),
            fLilypondCodeStream);

          switch (gGlobalLpsr2lilypondOahGroup->getLyricsDurationsKind ()) {
            case kLyricsDurationsImplicit:
              // don't generate a duration for automatic lyrics durations
              break;
            case kLyricsDurationsExplicit:
              fLilypondCodeStream <<
                elt->syllableWholeNotesAsMsrString ();
              break;
          } // switch

          fLilypondCodeStream <<
            " -- ";

#ifdef TRACING_IS_ENABLED
          if (gGlobalTraceOahGroup->getTraceLyrics ()) {
            fLilypondCodeStream <<
              "%{ syllableMiddle %} ";
          }
#endif
          break;

        case msrSyllable::kSyllableEnd:
          writeTextsListAsLilypondString (
            elt->getSyllableTextsList (),
            fLilypondCodeStream);

          switch (gGlobalLpsr2lilypondOahGroup->getLyricsDurationsKind ()) {
            case kLyricsDurationsImplicit:
              // don't generate a duration for automatic lyrics durations
              break;
            case kLyricsDurationsExplicit:
              fLilypondCodeStream <<
                elt->syllableWholeNotesAsMsrString ();
              break;
          } // switch

          fLilypondCodeStream <<
            ' ';

#ifdef TRACING_IS_ENABLED
          if (gGlobalTraceOahGroup->getTraceLyrics ()) {
            fLilypondCodeStream <<
              "%{ syllableEnd %} ";
          }
#endif
          break;

        case msrSyllable::kSyllableOnRestNote:
          // generate the syllable in lyrics for rests with syllables
          writeTextsListAsLilypondString (
            elt->getSyllableTextsList (),
            fLilypondCodeStream);
          fLilypondCodeStream <<
            elt->syllableWholeNotesAsMsrString () <<
            ' ';
#ifdef TRACING_IS_ENABLED
          if (gGlobalTraceOahGroup->getTraceLyrics ()) {
            fLilypondCodeStream <<
            " %{ syllableOnRestNote %} ";
          }
#endif
          break;

        case msrSyllable::kSyllableSkipRestNote:
          switch (gGlobalLpsr2lilypondOahGroup->getLyricsDurationsKind ()) {
            case kLyricsDurationsImplicit:
              // LilyPond ignores the skip durations when \lyricsto is used
#ifdef TRACING_IS_ENABLED
              if (gGlobalTraceOahGroup->getTraceLyrics ()) {
                fLilypondCodeStream <<
                  " %{ NOTHING for kSyllableSkipRestNote " <<
                  elt->syllableWholeNotesAsMsrString () <<
                  " %} ";
              }
#endif
              break;
            case kLyricsDurationsExplicit:
              fLilypondCodeStream <<
                "\\skip" <<
                elt->syllableWholeNotesAsMsrString () <<
                ' ';
#ifdef TRACING_IS_ENABLED
              if (gGlobalTraceOahGroup->getTraceLyrics ()) {
                fLilypondCodeStream <<
                " %{ kLyricsDurationsExplicit %} ";
              }
#endif
              break;
          } // switch
          break;

        case msrSyllable::kSyllableSkipNonRestNote:
          switch (gGlobalLpsr2lilypondOahGroup->getLyricsDurationsKind ()) {
            case kLyricsDurationsImplicit:
              // LilyPond ignores the skip durations when \lyricsto is used
              fLilypondCodeStream <<
                "\\skip" <<
                elt->syllableWholeNotesAsMsrString () <<
                ' ';
#ifdef TRACING_IS_ENABLED
              if (gGlobalTraceOahGroup->getTraceLyrics ()) {
                fLilypondCodeStream <<
                  " %{ syllableSkipNonRestNote lyricsDurationsImplicit %} ";
              }
#endif
              break;
            case kLyricsDurationsExplicit:
              fLilypondCodeStream <<
                "\\skip" <<
                elt->syllableWholeNotesAsMsrString () <<
                ' ';
#ifdef TRACING_IS_ENABLED
              if (gGlobalTraceOahGroup->getTraceLyrics ()) {
                fLilypondCodeStream <<
                " %{ syllableSkipNonRestNote lyricsDurationsExplicit %} ";
              }
#endif
              break;
          } // switch
          break;

        case msrSyllable::kSyllableMeasureEnd:
      // JMI      "| " <<
          if (gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()) {
            // print the measure end line number as a comment
            fLilypondCodeStream <<
              "%{ syllableMeasureEnd, line " <<
              elt->getInputLineNumber () <<
              " %}";
          }

      // JMI    fLilypondCodeStream << endl;
          break;

        case msrSyllable::kSyllableLineBreak:
          // print the measure end line number as a comment
          fLilypondCodeStream <<
            "%{ syllableLineBreak, line " <<
            elt->getInputLineNumber () <<
            " %} " <<
            "%| % " << // JMI BLARK TO BE FIXED
            elt->getSyllableNextMeasurePuristNumber () <<
            endl;
          break;

        case msrSyllable::kSyllablePageBreak:
          // print the measure end line number as a comment
          fLilypondCodeStream <<
            "%{ syllablePageBreak, line " <<
            elt->getInputLineNumber () <<
            " %}" <<
            endl;
          break;
      } // switch

      switch (elt->getSyllableExtendKind ()) {
        case msrSyllable::kSyllableExtendNone:
#ifdef TRACING_IS_ENABLED
          if (gGlobalTraceOahGroup->getTraceLyrics ()) {
            fLilypondCodeStream <<
              "%{ syllableExtendNone %} ";
          }
#endif
          break;

        case msrSyllable::kSyllableExtendEmpty:
#ifdef TRACING_IS_ENABLED
          if (gGlobalTraceOahGroup->getTraceLyrics ()) {
            fLilypondCodeStream <<
              "%{ syllableExtendEmpty %} ";
          }
#endif
          break;

        case msrSyllable::kSyllableExtendSingle:
          switch (gGlobalLpsr2lilypondOahGroup->getLyricsDurationsKind ()) {
            case kLyricsDurationsImplicit:
              // generate a lyric extender, i.e. a melisma, after this syllable
              fLilypondCodeStream <<
                "__ ";
              break;
            case kLyricsDurationsExplicit:
              // generate a lyric extender, i.e. a melisma, after this syllable ??? JMI
              fLilypondCodeStream <<
                "__ ";
              break;
          } // switch
#ifdef TRACING_IS_ENABLED
          if (gGlobalTraceOahGroup->getTraceLyrics ()) {
            fLilypondCodeStream <<
              "%{ syllableExtendSingle %} ";
          }
#endif
          break;

        case msrSyllable::kSyllableExtendStart:
          switch (gGlobalLpsr2lilypondOahGroup->getLyricsDurationsKind ()) {
            case kLyricsDurationsImplicit:
              // generate a lyric extender, i.e. a melisma, after this syllable
              fLilypondCodeStream <<
                "__ ";
              break;
            case kLyricsDurationsExplicit:
              // generate a lyric extender, i.e. a melisma, after this syllable ??? JMI
              fLilypondCodeStream <<
                "__ ";
              break;
          } // switch
#ifdef TRACING_IS_ENABLED
          if (gGlobalTraceOahGroup->getTraceLyrics ()) {
            fLilypondCodeStream <<
              "%{ syllableExtendStart %} ";
          }
#endif
          break;

        case msrSyllable::kSyllableExtendContinue:
#ifdef TRACING_IS_ENABLED
          if (gGlobalTraceOahGroup->getTraceLyrics ()) {
            fLilypondCodeStream <<
              "%{ syllableExtendContinue %} ";
          }
#endif
          break;

        case msrSyllable::kSyllableExtendStop:
#ifdef TRACING_IS_ENABLED
          if (gGlobalTraceOahGroup->getTraceLyrics ()) {
            fLilypondCodeStream <<
              "%{ syllableExtendStop %} ";
          }
#endif
          break;
      } // switch

    if (
      gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()
        ||
      gGlobalLpsr2lilypondOahGroup->getGeneratePositionsInMeasures ()
    ) {
      generateInputLineNumberAndOrPositionInMeasureAsAComment (
        elt);
      }
    }
  }
}

void lpsr2lilypondTranslator::visitEnd (S_msrSyllable& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrSyllable '" <<
      elt->asString () <<
      "'" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrClef& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrClef '" <<
      elt->asString () <<
      "'" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream << endl;

  // get the clef kind
  msrClefKind
    clefKind =
      elt->getClefKind ();

  if (gGlobalLpsr2lilypondOahGroup->getCommentClefChanges ()) {
  /* JMI
    S_msrClef
      currentVoiceCurrentClef =
        fCurrentVoice->getVoiceCurrentClef ();
*/

    if (
      fCurrentVoiceClef
        &&
      fCurrentVoiceClef->getClefKind () != clefKind
    ) {
      // this is a clef change, comment it

#ifdef TRACING_IS_ENABLED
      if (gGlobalTraceOahGroup->getTraceClefs ()) {
        gLogStream <<
          "Commenting clef change from " <<
          fCurrentVoiceClef->asShortString () <<
          " to " <<
          elt->asShortString () <<
          endl;
      }
#endif

      fLilypondCodeStream << "% ";
    }
  }

  if (clefKind != k_NoClef) {
    fLilypondCodeStream <<
      "\\clef ";

    switch (clefKind) {
      case k_NoClef:
        break;
      case kTrebleClef:
        fLilypondCodeStream << "\"treble\"";
        break;
      case kSopranoClef:
        fLilypondCodeStream << "\"soprano\"";
        break;
      case kMezzoSopranoClef:
        fLilypondCodeStream << "\"mezzosoprano\"";
        break;
      case kAltoClef:
        fLilypondCodeStream << "\"alto\"";
        break;
      case kTenorClef:
        fLilypondCodeStream << "\"tenor\"";
        break;
      case kBaritoneClef:
        fLilypondCodeStream << "\"baritone\"";
        break;
      case kBassClef:
        fLilypondCodeStream << "\"bass\"";
        break;
      case kTrebleLine1Clef:
        fLilypondCodeStream << "\"french\"";
        break;
      case kTrebleMinus15Clef:
        fLilypondCodeStream << "\"treble_15\"";
        break;
      case kTrebleMinus8Clef:
        fLilypondCodeStream << "\"treble_8\"";
        break;
      case kTreblePlus8Clef:
        fLilypondCodeStream << "\"treble^8\"";
        break;
      case kTreblePlus15Clef:
        fLilypondCodeStream << "\"treble^15\"";
        break;
      case kBassMinus15Clef:
        fLilypondCodeStream << "\"bass_15\"";
        break;
      case kBassMinus8Clef:
        fLilypondCodeStream << "\"bass_8\"";
        break;
      case kBassPlus8Clef:
        fLilypondCodeStream << "\"bass^8\"";
        break;
      case kBassPlus15Clef:
        fLilypondCodeStream << "\"bass^15\"";
        break;
      case kVarbaritoneClef:
        fLilypondCodeStream << "\"varbaritone\"";
        break;

      case kTablature4Clef:
      case kTablature5Clef:
      case kTablature6Clef:
      case kTablature7Clef:
        if (gGlobalLpsr2lilypondOahGroup->getModernTab ()) {
          fLilypondCodeStream <<
            "\"moderntab\"" <<
            endl;
        }
        else {
          fLilypondCodeStream <<
            "\"tab\"" <<
            endl;
        }
        if (gGlobalLpsr2lilypondOahGroup->getTabFullNotation ()) {
          fLilypondCodeStream <<
            "\\tabFullNotation" <<
            endl;
        }
        break;

      case kPercussionClef:
        fLilypondCodeStream << "\"percussion\"";
        break;
      case kJianpuClef:
        fLilypondCodeStream << "\"%{jianpuClef???%}\"";
        break;
    } // switch

  fLilypondCodeStream <<
    endl;
  }

  // register current voice clef
  fCurrentVoiceClef = elt;
}

void lpsr2lilypondTranslator::visitEnd (S_msrClef& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrClef '" <<
      elt->asString () <<
      "'" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrKey& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrKey '" <<
      elt->asString () <<
      "'" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  switch (fCurrentVoice->getVoiceKind ()) {
    case msrVoice::kVoiceRegular:
    case msrVoice::kVoiceHarmony:
      switch (elt->getKeyKind ()) {
        case msrKey::kTraditionalKind:
          fLilypondCodeStream <<
            "\\key " <<
            msrQuarterTonesPitchKindAsString (
              gGlobalLpsrOahGroup->getLpsrQuarterTonesPitchesLanguageKind (),
              elt->getKeyTonicQuarterTonesPitchKind ()) <<
            " \\" <<
            msrKey::keyModeKindAsString (
              elt->getKeyModeKind ()) <<
            endl;
          break;

        case msrKey::kHumdrumScotKind:
          {
            const vector<S_msrHumdrumScotKeyItem>&
              humdrumScotKeyItemsVector =
                elt->getHumdrumScotKeyItemsVector ();

            if (humdrumScotKeyItemsVector.size ()) {
              fLilypondCodeStream <<
                endl <<
                "\\set Staff.keyAlterations = #`(";

              vector<S_msrHumdrumScotKeyItem>::const_iterator
                iBegin = humdrumScotKeyItemsVector.begin (),
                iEnd   = humdrumScotKeyItemsVector.end (),
                i      = iBegin;

              for ( ; ; ) {
                S_msrHumdrumScotKeyItem item = (*i);

                if (elt->getKeyItemsOctavesAreSpecified ()) {
                  //   JMI   "((octave . step) . alter) ((octave . step) . alter) ...)";
                  //\set Staff.keyAlterations = #`(((3 . 3) . 7) ((3 . 5) . 3) ((3 . 6) . 3))"  \time 2/4


                    fLilypondCodeStream <<
                      "(" <<
                        "(" <<
                        item->getKeyItemOctave () - 3 <<
                          // in MusicXML, octave number is 4 for the octave
                          // starting with middle C,
                          // and the latter is c' in LilyPond
                        " . " <<
                        item->getKeyItemDiatonicPitchKind () <<
                        ")" <<
                      " . ," <<
                      alterationKindAsLilypondString (
                        item->getKeyItemAlterationKind ()) <<
                      ")";
                }

                else {
                  // Alternatively, for each item in the list, using the more concise format (step . alter) specifies that the same alteration should hold in all octaves.

                    fLilypondCodeStream <<
                      "(" <<
                      item->getKeyItemDiatonicPitchKind () <<
                      " . ," <<
                      alterationKindAsLilypondString (
                        item->getKeyItemAlterationKind ()) <<
                      ")";
                }

                if (++i == iEnd) break;

                fLilypondCodeStream << ' ';
              } // for

              fLilypondCodeStream <<
                ")";
            }

            else {
                msrInternalError (
                  gGlobalOahOahGroup->getInputSourceName (),
                  elt->getInputLineNumber (),
                  __FILE__, __LINE__,
                  "Humdrum/Scot key items vector is empty");
            }
          }
          break;
      } // switch
      break;

    case msrVoice::kVoiceFiguredBass:
      // not \key should be generated in \figuremode
      break;
  } // switch
}

void lpsr2lilypondTranslator::visitEnd (S_msrKey& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrKey '" <<
      elt->asString () <<
      "'"  <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrTime& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrTime " <<
      elt->asString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  msrTime::msrTimeSymbolKind
    timeSymbolKind =
      elt->getTimeSymbolKind ();

  const vector<S_msrTimeItem>&
    timeItemsVector =
      elt->getTimeItemsVector ();

/* JMI
  // is this the end of a senza misura fragment?
  if (
    fVoiceIsCurrentlySenzaMisura
      &&
    timeSymbolKind != msrTime::kTimeSymbolSenzaMisura) {
    fLilypondCodeStream <<
      "\\undo\\omit Staff.TimeSignature" <<
      endl;

    fVoiceIsCurrentlySenzaMisura = false;
  }
*/

  // handle the time
  if (timeSymbolKind == msrTime::kTimeSymbolSenzaMisura) {
    // senza misura time

    /* JMI
    fLilypondCodeStream <<
      "\\omit Staff.TimeSignature" <<
      endl;
*/

    fVoiceIsCurrentlySenzaMisura = true;
  }

  else {
    // con misura time

    int timesItemsNumber =
      timeItemsVector.size ();

    if (timesItemsNumber) {
      // should there be a single number?
      switch (timeSymbolKind) {
        case msrTime::kTimeSymbolCommon:
          break;
        case msrTime::kTimeSymbolCut:
          break;
        case msrTime::kTimeSymbolNote:
          break;
        case msrTime::kTimeSymbolDottedNote:
          break;
        case msrTime::kTimeSymbolSingleNumber:
          fLilypondCodeStream <<
            "\\once\\override Staff.TimeSignature.style = #'single-digit" <<
            endl;
          break;
        case msrTime::kTimeSymbolSenzaMisura:
          break;
        case msrTime::kTimeSymbolNone:
          break;
      } // switch

      if (! elt->getTimeIsCompound ()) {
        // simple time
        // \time "3/4" for 3/4
        // or senza misura

        S_msrTimeItem
          timeItem =
            timeItemsVector [0]; // the only element;

        // fetch the time item beat numbers vector
        const vector<int>&
          beatsNumbersVector =
            timeItem->
              getTimeBeatsNumbersVector ();

        // should the time be numeric?
        if (
          timeSymbolKind == msrTime::kTimeSymbolNone
            ||
          gGlobalLpsr2lilypondOahGroup->getNumericalTime ()) {
          fLilypondCodeStream <<
            "\\numericTimeSignature ";
        }

        fLilypondCodeStream <<
          "\\time " <<
          beatsNumbersVector [0] << // the only element
          "/" <<
          timeItem->getTimeBeatValue () <<
          endl;
      }

      else {
        // compound time
        // \compoundMeter #'(3 2 8) for 3+2/8
        // \compoundMeter #'((3 8) (2 8) (3 4)) for 3/8+2/8+3/4
        // \compoundMeter #'((3 2 8) (3 4)) for 3+2/8+3/4

        fLilypondCodeStream <<
          "\\compoundMeter #`(";

        // handle all the time items in the vector
        for (int i = 0; i < timesItemsNumber; i++) {
          S_msrTimeItem
            timeItem =
              timeItemsVector [i];

          // fetch the time item beat numbers vector
          const vector<int>&
            beatsNumbersVector =
              timeItem->
                getTimeBeatsNumbersVector ();

          int beatsNumbersNumber =
            beatsNumbersVector.size ();

          // first generate the opening parenthesis
          fLilypondCodeStream <<
            "(";

          // then generate all beats numbers in the vector
          for (int j = 0; j < beatsNumbersNumber; j++) {
            fLilypondCodeStream <<
              beatsNumbersVector [j] <<
              ' ';
          } // for

          // then generate the beat type
          fLilypondCodeStream <<
            timeItem->getTimeBeatValue ();

          // and finally generate the closing parenthesis
          fLilypondCodeStream <<
            ")";

          if (i != timesItemsNumber - 1) {
            fLilypondCodeStream <<
              ' ';
          }
        } // for

      fLilypondCodeStream <<
        ")" <<
        endl;
      }
    }

    else {
      // there are no time items
      if (timeSymbolKind != msrTime::kTimeSymbolSenzaMisura) {
        msrInternalError (
          gGlobalOahOahGroup->getInputSourceName (),
          elt->getInputLineNumber (),
          __FILE__, __LINE__,
          "time items vector is empty");
      }
    }
  }
}

void lpsr2lilypondTranslator::visitEnd (S_msrTime& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrTime " <<
      elt->asString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrTranspose& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrTranspose" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  int inputLineNumber =
    elt->getInputLineNumber ();

  int  transposeDiatonic  = elt->getTransposeDiatonic ();
  int  transposeChromatic = elt->getTransposeChromatic ();
  int  transposeOctaveChange = elt->getTransposeOctaveChange ();
  bool transposeDouble = elt->getTransposeDouble ();

/*
  // transposition in LilyPond is relative to c',
  // i.e. the C in the middle of the piano keyboard

The diatonic element specifies the number of pitch steps needed to go from written to sounding pitch. This allows for correct spelling of enharmonic transpositions.

The chromatic element represents the number of semitones needed to get from written to sounding pitch. This value does not include octave-change values; the values for both elements need to be added to the written pitch to get the correct sounding pitch.

The octave-change element indicates how many octaves to add to get from written pitch to sounding pitch.

If the double element is present, it indicates that the music is doubled one octave down from what is currently written (as is the case for mixed cello / bass parts in orchestral literature).
*/

  // determine transposition pitch
  msrQuarterTonesPitchKind
    transpositionPitchKind = k_NoQuarterTonesPitch_QTP;

  switch (transposeChromatic) {
    case -11:
      switch (transposeDiatonic) {
        case -7:
          transpositionPitchKind = kC_Sharp_QTP;
          break;

        case -6:
          transpositionPitchKind = kD_Flat_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case -10:
      switch (transposeDiatonic) {
        case -6:
          transpositionPitchKind = kD_Natural_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case -9:
      switch (transposeDiatonic) {
        case -6:
          transpositionPitchKind = kD_Sharp_QTP;
          break;

        case -5:
          transpositionPitchKind = kE_Flat_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case -8:
      switch (transposeDiatonic) {
        case -5:
          transpositionPitchKind = kE_Natural_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case -7:
      switch (transposeDiatonic) {
        case -4:
          transpositionPitchKind = kF_Natural_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case -6:
      switch (transposeDiatonic) {
        case -4:
          transpositionPitchKind = kF_Sharp_QTP;
          break;

        case -3:
          transpositionPitchKind = kG_Flat_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case -5:
      switch (transposeDiatonic) {
        case -3:
          transpositionPitchKind = kG_Natural_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case -4:
      switch (transposeDiatonic) {
        case -3:
          transpositionPitchKind = kG_Sharp_QTP;
          break;

        case -2:
          transpositionPitchKind = kA_Flat_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case -3:
      switch (transposeDiatonic) {
        case -2:
          transpositionPitchKind = kA_Natural_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case -2:
      switch (transposeDiatonic) {
        case -2:
          transpositionPitchKind = kA_Sharp_QTP;
          break;

        case -1:
          transpositionPitchKind = kB_Flat_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case -1:
      switch (transposeDiatonic) {
        case -1:
          transpositionPitchKind = kB_Natural_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case 0:
      switch (transposeDiatonic) {
        case 0:
          transpositionPitchKind = kC_Natural_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case 1:
      switch (transposeDiatonic) {
        case 0:
          transpositionPitchKind = kC_Sharp_QTP;
          break;

        case 1:
          transpositionPitchKind = kD_Flat_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case 2:
      switch (transposeDiatonic) {
        case 1:
          transpositionPitchKind = kD_Natural_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case 3:
      switch (transposeDiatonic) {
        case 1:
          transpositionPitchKind = kD_Sharp_QTP;
          break;

        case 2:
          transpositionPitchKind = kE_Flat_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case 4:
      switch (transposeDiatonic) {
        case 2:
          transpositionPitchKind = kE_Natural_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case 5:
      switch (transposeDiatonic) {
        case 3:
          transpositionPitchKind = kF_Natural_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case 6:
      switch (transposeDiatonic) {
        case 3:
          transpositionPitchKind = kF_Sharp_QTP;
          break;

        case 4:
          transpositionPitchKind = kG_Flat_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case 7:
      switch (transposeDiatonic) {
        case 4:
          transpositionPitchKind = kG_Natural_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case 8:
      switch (transposeDiatonic) {
        case 4:
          transpositionPitchKind = kG_Sharp_QTP;
          break;

        case 5:
          transpositionPitchKind = kA_Flat_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case 9:
      switch (transposeDiatonic) {
        case 5:
          transpositionPitchKind = kA_Natural_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case 10:
      switch (transposeDiatonic) {
        case 5:
          transpositionPitchKind = kA_Sharp_QTP;
          break;

        case 6:
          transpositionPitchKind = kB_Flat_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    case 11:
      switch (transposeDiatonic) {
        case 6:
          transpositionPitchKind = kB_Natural_QTP;
          break;

        default:
          transposeDiatonicError (
            inputLineNumber,
            transposeDiatonic,
            transposeChromatic);
      } // switch
      break;

    default:
      {
        stringstream s;

        s <<
          "transpose chromatic '" << transposeChromatic <<
          "' is not between -12 and 12, ignored";

        msrMusicXMLError (
          gGlobalOahOahGroup->getInputSourceName (),
          elt->getInputLineNumber (),
          __FILE__, __LINE__,
          s.str ());
      }
  } // switch

  // 4 is the octave starting with middle C
  int transpositionOctave;

  if (transposeChromatic < 0) {
    transpositionOctave = 3;
  }
  else {
    transpositionOctave = 4;
  }

  // take the transpose octave change into account
  transpositionOctave += transposeOctaveChange;

  // take the transpose double if any into account
  if (transposeDouble) {
    transpositionOctave--;
  }

  string
    transpositionPitchKindAsString =
      msrQuarterTonesPitchKindAsString (
        gGlobalLpsrOahGroup->
          getLpsrQuarterTonesPitchesLanguageKind (),
        transpositionPitchKind);

  string
    transpositionOctaveAsString =
      absoluteOctaveAsLilypondString (
        transpositionOctave);

/* JMI
#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceTranspositions ()) {
    fLilypondCodeStream << // JMI
      "Handlling transpose '" <<
      elt->transposeAsString () <<
      "' ignored because it is already present in voice \"" <<
      fCurrentVoice->getVoiceName () <<
      "\"" <<
      / * JMI
      getStaffName () <<
      "\" in part " <<
      fStaffPartUpLink->getPartCombinedName () <<
      * /
      endl <<
      ", transpositionPitch: " <<
      transpositionPitchAsString <<
      ", transpositionOctave: " <<
      transpositionOctaveAsString <<
      "(" << transpositionOctave << ")" <<
      endl;
    }
#endif
*/

  // now we can generate the transpostion command
  fLilypondCodeStream <<
    "\\transposition " <<
    transpositionPitchKindAsString <<
    transpositionOctaveAsString <<
    ' ' <<
    endl;
}

void lpsr2lilypondTranslator::visitEnd (S_msrTranspose& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrTranspose" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrPartNameDisplay& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrPartNameDisplay" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  string partNameDisplayValue =
    elt->getPartNameDisplayValue ();

  fLilypondCodeStream <<
    "\\set Staff.instrumentName = \"" <<
    partNameDisplayValue <<
    "\"" <<
    endl <<
 // JMI ???   "\\tweak break-visibility #end-of-line-visible" <<
 //   endl <<
    "\\tweak RehearsalMark.self-alignment-X #LEFT" <<
    endl <<
    "\\mark\\markup{\"" <<
    partNameDisplayValue <<
    "\"}" <<
    endl;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrPartAbbreviationDisplay& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrPartAbbreviationDisplay" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  string partAbbreviationDisplayValue =
    elt->getPartAbbreviationDisplayValue ();

  fLilypondCodeStream <<
    "\\set Staff.shortInstrumentName = " <<
    nameAsLilypondString (partAbbreviationDisplayValue) <<
    endl;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrTempo& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrTempo" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  int inputLineNumber =
    elt->getInputLineNumber ();

  const list<S_msrWords>&
    tempoWordsList =
      elt->getTempoWordsList ();

  int tempoWordsListSize = tempoWordsList.size ();

  msrDottedDuration tempoBeatUnit  = elt->getTempoBeatUnit ();
  string            tempoPerMinute = elt->getTempoPerMinute ();

  msrTempo::msrTempoParenthesizedKind
    tempoParenthesizedKind =
      elt->getTempoParenthesizedKind ();

  switch (elt->getTempoPlacementKind ()) {
    case msrPlacementKind::kPlacementNone:
      break;
    case msrPlacementKind::kPlacementAbove:
      // by default, so nothing to do
      break;
    case msrPlacementKind::kPlacementBelow:
      fLilypondCodeStream <<
        "\\once\\override Score.MetronomeMark.direction = #DOWN" <<
        endl;
      break;
    } // switch

  switch (elt->getTempoKind ()) {
    case msrTempo::k_NoTempoKind:
      break;

    case msrTempo::kTempoBeatUnitsWordsOnly:
        fLilypondCodeStream <<
          "\\tempo ";

        if (tempoWordsListSize) {
          list<S_msrWords>::const_iterator
            iBegin = tempoWordsList.begin (),
            iEnd   = tempoWordsList.end (),
            i      = iBegin;

          for ( ; ; ) {
            S_msrWords words = (*i);

            fLilypondCodeStream <<
              "\"" << words->getWordsContents () << "\"";

            if (++i == iEnd) break;

            fLilypondCodeStream <<
              ' ';
          } // for

          fLilypondCodeStream <<
            endl;
        }
      break;

    case msrTempo::kTempoBeatUnitsPerMinute:
      switch (tempoParenthesizedKind) {
        case msrTempo::kTempoParenthesizedYes:
          fLilypondCodeStream <<
            "\\tempo " <<
            "\\markup {" <<
            endl;

          gIndenter++;

          fLilypondCodeStream <<
            "\\concat {" <<
            endl;

          gIndenter++;

          gIndenter++;

          if (tempoWordsListSize) {
            list<S_msrWords>::const_iterator
              iBegin = tempoWordsList.begin (),
              iEnd   = tempoWordsList.end (),
              i      = iBegin;

            for ( ; ; ) {
              S_msrWords words = (*i);

              fLilypondCodeStream <<
                "\"" << words->getWordsContents () << "\"";

              if (++i == iEnd) break;

              fLilypondCodeStream <<
                ' ';
            } // for
          }

          fLilypondCodeStream <<
            "(";
          if (gGlobalLpsrOahGroup->versionNumberGreaterThanOrEqualTo ("2.20")) {
            fLilypondCodeStream <<
              " \\smaller \\general-align #Y #DOWN \\note {";
          }
          else {
            fLilypondCodeStream <<
              "\\smaller \\general-align #Y #DOWN \\note #\"";
          }

          fLilypondCodeStream <<
            dottedDurationAsLilypondStringWithoutBackSlash (
              inputLineNumber,
              tempoBeatUnit);

          if (gGlobalLpsrOahGroup->versionNumberGreaterThanOrEqualTo ("2.20")) {
            fLilypondCodeStream <<
              "} #UP";
          }
          else {
            fLilypondCodeStream <<
              "\" #UP";
          }

          fLilypondCodeStream <<
            endl <<
            "\" = \"" <<
            endl <<
            tempoPerMinute <<
            ")" <<
            endl;

          gIndenter--;

          fLilypondCodeStream << endl;

          gIndenter--;

          fLilypondCodeStream <<
            "}" << "% concat" <<
            endl;

          gIndenter--;

          fLilypondCodeStream <<
            "}" <<
            endl;
        break;

      case msrTempo::kTempoParenthesizedNo:
        fLilypondCodeStream <<
          "\\tempo " <<
          "\\markup {" <<
          endl;

        gIndenter++;

        if (tempoWordsListSize) {
          list<S_msrWords>::const_iterator
            iBegin = tempoWordsList.begin (),
            iEnd   = tempoWordsList.end (),
            i      = iBegin;

          for ( ; ; ) {
            S_msrWords words = (*i);

            fLilypondCodeStream <<
              "\"" << words->getWordsContents () << "\"";

            if (++i == iEnd) break;

            fLilypondCodeStream <<
              ' ';
          } // for
        }

        fLilypondCodeStream <<
          "\\concat {" <<
          endl;

        gIndenter++;

        if (gGlobalLpsrOahGroup->versionNumberGreaterThanOrEqualTo ("2.20")) {
          fLilypondCodeStream <<
            " \\smaller \\general-align #Y #DOWN \\note {";
        }
        else {
          fLilypondCodeStream <<
            " \\smaller \\general-align #Y #DOWN \\note #\"";
        }

        fLilypondCodeStream <<
          dottedDurationAsLilypondStringWithoutBackSlash (
            inputLineNumber,
            tempoBeatUnit);

        if (gGlobalLpsrOahGroup->versionNumberGreaterThanOrEqualTo ("2.20")) {
          fLilypondCodeStream <<
            "} #UP";
        }
        else {
          fLilypondCodeStream <<
            "\" #UP";
        }

        fLilypondCodeStream <<
          endl <<
          "\" = \"" <<
          endl <<
          tempoPerMinute <<
        endl;

      gIndenter--;

      fLilypondCodeStream <<
        "}" << "% concat" <<
        endl;

      gIndenter--;

      fLilypondCodeStream <<
        "}" <<
        endl;
      } // switch
      break;

    case msrTempo::kTempoBeatUnitsEquivalence:
      fLilypondCodeStream <<
        "\\tempo ";

      if (tempoWordsListSize) {
        list<S_msrWords>::const_iterator
          iBegin = tempoWordsList.begin (),
          iEnd   = tempoWordsList.end (),
          i      = iBegin;

        for ( ; ; ) {
          S_msrWords words = (*i);

          fLilypondCodeStream <<
            "\"" << words->getWordsContents () << "\"";

          if (++i == iEnd) break;

          fLilypondCodeStream <<
            ' ';
        } // for
      }

      fLilypondCodeStream <<
        ' ' <<
        "\\markup {" <<
        endl;

      gIndenter++;

      fLilypondCodeStream <<
        "\\concat {" <<
        endl;

      gIndenter++;

      switch (tempoParenthesizedKind) {
        case msrTempo::kTempoParenthesizedYes:
          fLilypondCodeStream <<
            "(" <<
            endl;
          break;
        case msrTempo::kTempoParenthesizedNo:
          break;
      } // switch

      gIndenter++;

      if (gGlobalLpsrOahGroup->versionNumberGreaterThanOrEqualTo ("2.20")) {
        fLilypondCodeStream <<
          " \\smaller \\general-align #Y #DOWN \\note {";
      }
      else {
        fLilypondCodeStream <<
          " \\smaller \\general-align #Y #DOWN \\note #\"";
      }

      fLilypondCodeStream <<
        dottedDurationAsLilypondStringWithoutBackSlash (
          inputLineNumber,
          tempoBeatUnit);

      if (gGlobalLpsrOahGroup->versionNumberGreaterThanOrEqualTo ("2.20")) {
        fLilypondCodeStream <<
          "} #UP";
      }
      else {
        fLilypondCodeStream <<
          "\" #UP";
      }

      fLilypondCodeStream <<
        endl <<
        "\" = \"" <<
        endl;

      fLilypondCodeStream <<
        "(";
      if (gGlobalLpsrOahGroup->versionNumberGreaterThanOrEqualTo ("2.20")) {
        fLilypondCodeStream <<
          " \\smaller \\general-align #Y #DOWN \\note {";
      }
      else {
        fLilypondCodeStream <<
          " \\smaller \\general-align #Y #DOWN \\note #\"";
      }

      fLilypondCodeStream <<
        dottedDurationAsLilypondStringWithoutBackSlash (
          inputLineNumber,
          elt->getTempoEquivalentBeatUnit ());

      if (gGlobalLpsrOahGroup->versionNumberGreaterThanOrEqualTo ("2.20")) {
        fLilypondCodeStream <<
          "} #UP";
      }
      else {
        fLilypondCodeStream <<
          "\" #UP";
      }

      fLilypondCodeStream << endl;

      gIndenter--;

      switch (tempoParenthesizedKind) {
        case msrTempo::kTempoParenthesizedYes:
          fLilypondCodeStream <<
            ")" <<
            endl;
          break;
        case msrTempo::kTempoParenthesizedNo:
          break;
      } // switch

      gIndenter--;

      fLilypondCodeStream <<
        "}" <<
        endl;

      gIndenter--;

      fLilypondCodeStream <<
        "}" <<
        endl;
      break;

    case msrTempo::kTempoNotesRelationShip:
      fLilypondCodeStream <<
        "\\tempoRelationship #\"";

      if (tempoWordsListSize) {
        list<S_msrWords>::const_iterator
          iBegin = tempoWordsList.begin (),
          iEnd   = tempoWordsList.end (),
          i      = iBegin;

        for ( ; ; ) {
          S_msrWords words = (*i);

          fLilypondCodeStream <<
     // JMI       "\"" <<
            words->getWordsContents (); // JMI <<
      // JMI      "\"";

          if (++i == iEnd) break;

          fLilypondCodeStream <<
            ' ';
        } // for
      }

      fLilypondCodeStream <<
        "\"";

      switch (tempoParenthesizedKind) {
        case msrTempo::kTempoParenthesizedYes:
          fLilypondCodeStream <<
            " ##t";
          break;
        case msrTempo::kTempoParenthesizedNo:
          fLilypondCodeStream <<
            " ##f";
          break;
      } // switch

      fLilypondCodeStream << endl;
      break;
  } // switch
}

void lpsr2lilypondTranslator::visitStart (S_msrTempoRelationshipElements& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrTempoRelationshipElements" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    "\\fixed b' {" <<
    endl;

  gIndenter++; // decremented in visitEnd (S_msrTempoRelationshipElements& elt)
}

void lpsr2lilypondTranslator::visitEnd (S_msrTempoRelationshipElements& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrTempoRelationshipElements" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  gIndenter--; // incremented in visitStart (S_msrTempoRelationshipElements& elt)

  fLilypondCodeStream <<
    endl <<
    "}" <<
    endl;
}

void lpsr2lilypondTranslator::visitStart (S_msrTempoNote& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrTempoNote" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    "b" <<
    wholeNotesAsLilypondString (
      elt->getInputLineNumber (),
      elt->getTempoNoteWholeNotes ()) <<
      ' ';
}

void lpsr2lilypondTranslator::visitStart (S_msrTempoTuplet& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrTempoTuplet" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  const msrTupletFactor&
    tempoTupletFactor =
      elt->getTempoTupletFactor ();

  fLilypondCodeStream <<
    "\\tuplet " <<
    tempoTupletFactor.getTupletActualNotes () <<
    "/" <<
    tempoTupletFactor.getTupletNormalNotes () << " { ";
}

void lpsr2lilypondTranslator::visitEnd (S_msrTempoTuplet& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrTempoTuplet" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    "}";
}

void lpsr2lilypondTranslator::visitEnd (S_msrTempo& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrTempo" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrArticulation& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrArticulation" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // don't generate the articulation here,
  // the note or chord will do it in its visitEnd () method
}

void lpsr2lilypondTranslator::visitEnd (S_msrArticulation& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrArticulation" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrFermata& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrFermata" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

/*
Articulations can be attached to rests as well as notes but they cannot be attached to multi-measure rests. A special predefined command, \fermataMarkup, is available for at- taching a fermata to a multi-measure rest (and only a multi-measure rest). This creates a MultiMeasureRestText object.
*/

/* JMI
  switch (elt->getArticulationPlacement ()) {
    case kPlacementNone:
      // nothing needed
      break;
    case kPlacementAbove:
      fLilypondCodeStream << "^";
      break;
    case kPlacementBelow:
      fLilypondCodeStream << "_";
      break;
  } // switch

  // don't generate fermatas for chord member notes
  if (false && fOnGoingNotesStack.size () > 0) { // JMI
    switch (elt->getFermataTypeKind ()) {
      case msrFermata::kFermataTypeNone:
        // no placement needed
        break;
      case msrFermata::kFermataTypeUpright:
        // no placement needed
        break;
      case msrFermata::kFermataTypeInverted:
        fLilypondCodeStream << "_";
        break;
    } // switch

    switch (elt->getFermataKind ()) {
      case msrFermata::kNormalFermataKind:
        fLilypondCodeStream << "\\fermata ";
        break;
      case msrFermata::kAngledFermataKind:
        fLilypondCodeStream << "\\shortfermata ";
        break;
      case msrFermata::kSquareFermataKind:
        fLilypondCodeStream << "\\longfermata ";
        break;
    } // switch
  }
*/
}

void lpsr2lilypondTranslator::visitEnd (S_msrFermata& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrFermata" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrArpeggiato& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrArpeggiato" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

/* VIRER JMI
  */
}

void lpsr2lilypondTranslator::visitEnd (S_msrArpeggiato& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrArpeggiato" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrNonArpeggiato& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrNonArpeggiato" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

/* VIRER JMI
  */
}

void lpsr2lilypondTranslator::visitEnd (S_msrNonArpeggiato& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrNonArpeggiato" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrTechnical& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrTechnical" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // don't generate the technical here,
  // the note or chord will do it in its visitEnd () method
}

void lpsr2lilypondTranslator::visitEnd (S_msrTechnical& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrTechnical" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrTechnicalWithInteger& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrTechnicalWithInteger" <<
      ", fOnGoingChord = " <<
      booleanAsString (fOnGoingChord) <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // don't generate the technicalWithInteger here,
  // the note or chord will do it in its visitEnd () method
}

void lpsr2lilypondTranslator::visitEnd (S_msrTechnicalWithInteger& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrTechnicalWithInteger" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrTechnicalWithFloat& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrTechnicalWithFloat" <<
      ", fOnGoingChord = " <<
      booleanAsString (fOnGoingChord) <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // don't generate the technicalWithFloat here,
  // the note or chord will do it in its visitEnd () method
}

void lpsr2lilypondTranslator::visitEnd (S_msrTechnicalWithFloat& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrTechnicalWithFloat" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrTechnicalWithString& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrTechnicalWithString" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // don't generate the technicalWithString here,
  // the note or chord will do it in its visitEnd () method
}

void lpsr2lilypondTranslator::visitEnd (S_msrTechnicalWithString& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrTechnicalWithString" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrOrnament& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrOrnament" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // don't generate the ornament here,
  // the note or chord will do it in its visitEnd () method
}

void lpsr2lilypondTranslator::visitEnd (S_msrOrnament& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrOrnament" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrGlissando& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrGlissando" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // don't generate the glissando here,
  // the note or chord will do it in its visitEnd () method
}

void lpsr2lilypondTranslator::visitEnd (S_msrGlissando& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrGlissando" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrSlide& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrSlide" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // don't generate the slide here,
  // the note or chord will do it in its visitEnd () method
}

void lpsr2lilypondTranslator::visitEnd (S_msrSlide& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrSlide" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrSingleTremolo& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrSingleTremolo" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // don't generate the singleTremolo here,
  // the note or chord will do it in its visitEnd () method
}

void lpsr2lilypondTranslator::visitEnd (S_msrSingleTremolo& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrSingleTremolo" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrDoubleTremolo& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrDoubleTremolo" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // get double tremolo number of repeats
  int numberOfRepeats =
    elt->getDoubleTremoloNumberOfRepeats ();

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceTremolos ()) {
    fLilypondCodeStream <<
      "% visitStart (S_msrDoubleTremolo&)" <<
      endl;

    gIndenter++;

    fLilypondCodeStream <<
      "% doubleTremoloSoundingWholeNotes = " <<
      elt->getDoubleTremoloSoundingWholeNotes () <<
      endl <<

      "% gdoubleTremoloElementsDuration = " <<
      elt->getDoubleTremoloElementsDuration () <<
      endl <<

      "% doubleTremoloMarksNumber = " <<
      elt->getDoubleTremoloMarksNumber () <<
      endl <<

      "% numberOfRepeats = " <<
      numberOfRepeats <<
      endl;

    gIndenter--;
  }
#endif

  fLilypondCodeStream <<
    "\\repeat tremolo " << numberOfRepeats << " {";

  gIndenter++; // decremented in visitEnd (S_msrDoubleTremolo& elt)
}

void lpsr2lilypondTranslator::visitEnd (S_msrDoubleTremolo& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrDoubleTremolo" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  gIndenter--; // incremented in visitStart (S_msrDoubleTremolo& elt)

  fLilypondCodeStream <<
    "}" <<
    endl;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrDynamics& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrDynamics" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrDynamics& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrDynamics" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrOtherDynamics& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrOtherDynamics" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrOtherDynamics& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrOtherDynamics" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrWords& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrWords" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrWords& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrWords" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrSlur& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrSlur " <<
      elt->asShortString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrSlur& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrSlur " <<
      elt->asShortString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

/*
void lpsr2lilypondTranslator::visitStart (S_msrChordSlurLink& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrChordSlurLink " <<
      elt->asShortString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrChordSlurLink& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrChordSlurLink " <<
      elt->asShortString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}
*/

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrLigature& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrLigature" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrLigature& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrLigature" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrWedge& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrWedge" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrWedge& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrWedge" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generateNoteBeams (S_msrNote note)
{
  const list<S_msrBeam>&
    noteBeams =
      note->getNoteBeams ();

  if (noteBeams.size ()) {
    for (
      list<S_msrBeam>::const_iterator i = noteBeams.begin ();
      i != noteBeams.end ();
      i++
    ) {
      S_msrBeam beam = (*i);

 #ifdef TRACING_IS_ENABLED
      if (gGlobalTraceOahGroup->getTraceBeams ()) {
        gLogStream <<
          "Considering to generate LilyPond code for beam " <<
          beam->asShortString () <<
          " in note " <<
          note->asShortString () <<
          endl;
      }
#endif

     // LilyPond will take care of multiple beams automatically,
      // so we need only generate code for the first number (level)
      switch (beam->getBeamKind ()) {

        case msrBeam::kBeginBeam:
          if (beam->getBeamNumber () == 1) {
            if (! gGlobalLpsr2lilypondOahGroup->getNoBeams ()) {
 #ifdef TRACING_IS_ENABLED
              if (gGlobalTraceOahGroup->getTraceBeams ()) {
                gLogStream <<
                  "Generating LilyPond code for beam " <<
                  beam->asShortString () <<
                  " in note " <<
                  note->asShortString () <<
                  endl;
              }
#endif

              fLilypondCodeStream << "[ ";

              if (gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()) {
                // print the input line number as a comment
                fLilypondCodeStream <<
                  "%{ line " << beam->getInputLineNumber () << "%} ";
              }
            }
          }
          break;

        case msrBeam::kContinueBeam:
          break;

        case msrBeam::kEndBeam:
          if (beam->getBeamNumber () == 1) {
            if (! gGlobalLpsr2lilypondOahGroup->getNoBeams ()) {
              fLilypondCodeStream << "] ";

              if (gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()) {
                // print the input line number as a comment
                fLilypondCodeStream <<
                  "%{ line " << beam->getInputLineNumber () << "%} ";
              }
            }
          }
          break;

        case msrBeam::kForwardHookBeam:
          break;

        case msrBeam::kBackwardHookBeam:
          break;

        case msrBeam::k_NoBeam:
          break;
      } // switch
    } // for
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generateNoteSlurs (S_msrNote note)
{
  const list<S_msrSlur>&
    noteSlurs =
      note->getNoteSlurs ();

  if (noteSlurs.size ()) {
    list<S_msrSlur>::const_iterator i;
    for (
      i=noteSlurs.begin ();
      i!=noteSlurs.end ();
      i++
    ) {
      S_msrSlur slur = (*i);

 #ifdef TRACING_IS_ENABLED
      if (gGlobalTraceOahGroup->getTraceSlurs ()) {
        gLogStream <<
          "Considering to generate LilyPond code for slur " <<
          slur->asShortString () <<
          " in note " <<
          note->asShortString () <<
          endl;
      }
#endif

      /* JMI ???
        \slurDashed, \slurDotted, \slurHalfDashed,
        \slurHalfSolid, \slurDashPattern, \slurSolid
      */

      switch (slur->getSlurTypeKind ()) {
        case msrSlur::k_NoSlur:
          break;
        case msrSlur::kRegularSlurStart:
 #ifdef TRACING_IS_ENABLED
          if (gGlobalTraceOahGroup->getTraceSlurs ()) {
            gLogStream <<
              "Generating LilyPond code for slur " <<
              slur->asShortString () <<
              " in note " <<
              note->asShortString () <<
              endl;
          }
#endif

          fLilypondCodeStream << "( ";

          if (gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()) {
            // print the input line number as a comment
            fLilypondCodeStream <<
              "%{ line " << slur->getInputLineNumber () << "%} ";
          }
          break;
        case msrSlur::kPhrasingSlurStart:
          fLilypondCodeStream << "\\( ";

          if (gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()) {
            // print the input line number as a comment
            fLilypondCodeStream <<
              "%{ line " << slur->getInputLineNumber () << "%} ";
          }
          break;
        case msrSlur::kSlurContinue:
          break;
        case msrSlur::kRegularSlurStop:
          fLilypondCodeStream << ") ";

          if (gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()) {
            // print the input line number as a comment
            fLilypondCodeStream <<
              "%{ line " << slur->getInputLineNumber () << "%} ";
          }
          break;
        case msrSlur::kPhrasingSlurStop:
          fLilypondCodeStream << "\\) ";

          if (gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()) {
            // print the input line number as a comment
            fLilypondCodeStream <<
              "%{ line " << slur->getInputLineNumber () << "%} ";
          }
          break;
      } // switch
    } // for
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generateGraceNotesGroup (
  S_msrGraceNotesGroup graceNotesGroup)
{
  /*
    1. no slash, no slur: \grace
    2. slash and slur: \acciaccatura, LilyPond will slur it JMI
    3. slash but no slur: \slashedGrace
    4. no slash but slur: \appoggiatura, LilyPond will slur it JMI
  */

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceGraceNotes ()) {
    fLilypondCodeStream <<
      "% --> generating code for grace notes group " <<
      graceNotesGroup->asString () <<
      ", line " << graceNotesGroup->getInputLineNumber () <<
      endl;
  }
#endif

  bool doGenerateASlurIfPresent = true;

  switch (graceNotesGroup->getGraceNotesGroupKind ()) {
    case msrGraceNotesGroup::kGraceNotesGroupBefore:
      if (graceNotesGroup->getGraceNotesGroupIsSlashed ()) {
        if (graceNotesGroup->getGraceNotesGroupIsTied ()) {
          fLilypondCodeStream <<
            "\\acciaccatura";
          doGenerateASlurIfPresent = false;
        }
        else {
          fLilypondCodeStream <<
            "\\slashedGrace";
        }
      }

      else {
        if (graceNotesGroup->getGraceNotesGroupIsTied ()) {
          fLilypondCodeStream <<
            "\\appoggiatura";
          doGenerateASlurIfPresent = false;
        }
        else {
          fLilypondCodeStream <<
            "\\grace";
        }
      }
      break;

    case msrGraceNotesGroup::kGraceNotesGroupAfter:
      // don't generate anything here
      break;
  } // switch

  fLilypondCodeStream <<
    " { ";

  // force durations to be displayed explicitly
  // at the beginning of the grace notes
  fLastMetWholeNotes = rational (0, 1);

  // generate the notes in the grace notes group
  const list<S_msrMeasureElement>&
    graceNotesGroupElementsList =
      graceNotesGroup->
        getGraceNotesGroupElementsList ();

  int
    graceNotesGroupElementsListSize =
      graceNotesGroupElementsList.size ();

  if (graceNotesGroupElementsListSize) {
    list<S_msrMeasureElement>::const_iterator
      iBegin = graceNotesGroupElementsList.begin (),
      iEnd   = graceNotesGroupElementsList.end (),
      i      = iBegin;

    int elementNumber = 0;

    for ( ; ; ) {
      S_msrElement element = (*i);

      elementNumber += 1;

      if (
        // note?
        S_msrNote
          note =
            dynamic_cast<msrNote*>(&(*element))
        ) {
          // generate things before the note
          generateCodeRightBeforeNote (note);

          // generate the note itself
          generateCodeForNote (note);

          if ( // JMI
            gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()
              ||
            gGlobalLpsr2lilypondOahGroup->getGeneratePositionsInMeasures ()
          ) {
            generateInputLineNumberAndOrPositionInMeasureAsAComment (
              note);
          }

          // generate things after the note
          generateCodeRightAfterNote (note);

          // generate the note beams if any,
          // unless the note is chord member
          if (! note->getNoteBelongsToAChord ()) {
            generateNoteBeams (note);
          }

          if (graceNotesGroup->getGraceNotesGroupIsBeamed ()) {
            if (elementNumber == 1) {
              fLilypondCodeStream << "[ ";
            }
            else if (elementNumber == graceNotesGroupElementsListSize) {
              fLilypondCodeStream << "] ";
            }
          }

          // generate the note slurs if any,
          // unless the note is chord member
          // or LilyPond will take care of that
          if (
            ! note->getNoteBelongsToAChord ()
              &&
            doGenerateASlurIfPresent
          ) {
            generateNoteSlurs (note);
          }
        }

      else if (
        // chord?
        S_msrChord
          chord =
            dynamic_cast<msrChord*>(&(*element))
        ) {
          generateChordInGraceNotesGroup (chord);
        }

      else {
        stringstream s;

        fLilypondCodeStream <<
          "grace notes group elements list in '" <<
          graceNotesGroup->asString () <<
          "' is empty" <<
          ", line " << graceNotesGroup->getInputLineNumber ();

        msrInternalError (
          gGlobalOahOahGroup->getInputSourceName (),
          graceNotesGroup->getInputLineNumber (),
          __FILE__, __LINE__,
          s.str ());
      }

      if (++i == iEnd) break;
      fLilypondCodeStream <<
        ' ';
    } // for

    fLilypondCodeStream << "} ";
  }

  else {
    stringstream s;

    fLilypondCodeStream <<
      "grace notes group elements list in '" <<
      graceNotesGroup->asString () <<
      "' is empty" <<
      ", line " << graceNotesGroup->getInputLineNumber ();

    msrInternalError (
      gGlobalOahOahGroup->getInputSourceName (),
      graceNotesGroup->getInputLineNumber (),
      __FILE__, __LINE__,
      s.str ());
  }

  // force durations to be displayed explicitly
  // at the end of the grace notes
  fLastMetWholeNotes = rational (0, 1);
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrGraceNotesGroup& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrGraceNotesGroup " <<
      elt->asShortString () <<
      ", fOnGoingChord: " << booleanAsString (fOnGoingChord) <<
      ", fOnGoingGraceNotesGroup: " << booleanAsString (fOnGoingGraceNotesGroup) <<
      ", fOnGoingChordGraceNotesGroupLink: " << booleanAsString (fOnGoingChordGraceNotesGroupLink) <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fOnGoingGraceNotesGroup = true;
}

void lpsr2lilypondTranslator::visitEnd (S_msrGraceNotesGroup& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrGraceNotesGroup " <<
      elt->asShortString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fOnGoingGraceNotesGroup = false;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrChordGraceNotesGroupLink& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrChordGraceNotesGroupLink " <<
      elt->asShortString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fOnGoingChordGraceNotesGroupLink = true;
}

void lpsr2lilypondTranslator::visitEnd (S_msrChordGraceNotesGroupLink& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrChordGraceNotesGroupLink " <<
      elt->asShortString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fOnGoingChordGraceNotesGroupLink = false;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrAfterGraceNotesGroup& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrAfterGraceNotesGroup " <<
      elt->asShortString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

 // JMI exists? if (elt->getGraceNotesGroupIsSlashed ()) {}
  fLilypondCodeStream <<
    "\\afterGrace { %{ visitStart (S_msrAfterGraceNotesGroup&) %} ";
}

void lpsr2lilypondTranslator::visitStart (S_msrAfterGraceNotesGroupContents& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrAfterGraceNotesGroupContents " <<
      elt->asShortString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  // write a first closing right bracket right now
  fLilypondCodeStream <<
    "} { ";

  // force durations to be displayed explicitly
  // at the beginning of the after grace notes contents
  fLastMetWholeNotes = rational (0, 1);
}

void lpsr2lilypondTranslator::visitEnd (S_msrAfterGraceNotesGroupContents& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrAfterGraceNotesGroupContents " <<
      elt->asShortString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    "} ";
}

void lpsr2lilypondTranslator::visitEnd (S_msrAfterGraceNotesGroup& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrAfterGraceNotesGroup " <<
      elt->asShortString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generateBeforeNoteSpannersIfAny (
  S_msrNote note)
{
  const list<S_msrSpanner>&
    noteSpanners =
      note->getNoteSpanners ();

  if (noteSpanners.size ()) {
    list<S_msrSpanner>::const_iterator i;
    for (
      i=noteSpanners.begin ();
      i!=noteSpanners.end ();
      i++
    ) {
      S_msrSpanner
        spanner = (*i);

      bool doGenerateSpannerCode = true;

      switch (spanner->getSpannerKind ()) {
        case msrSpanner::kSpannerDashes:
          break;
        case msrSpanner::kSpannerWavyLine:
          if (spanner->getSpannerNoteUpLink ()->getNoteTrillOrnament ()) {
            // don't generate anything, the trill will display the wavy line
            doGenerateSpannerCode = false;
          }
          break;
      } // switch

      if (doGenerateSpannerCode) {
        switch (spanner->getSpannerKind ()) {
          case msrSpanner::kSpannerDashes:
          case msrSpanner::kSpannerWavyLine:
            break;
        } // switch

        generateCodeForSpannerBeforeNote (spanner);
      }
    } // for
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generateAfterNoteSpannersIfAny (
  S_msrNote note)
{
  const list<S_msrSpanner>&
    noteSpanners =
      note->getNoteSpanners ();

  if (noteSpanners.size ()) {
    list<S_msrSpanner>::const_iterator i;
    for (
      i=noteSpanners.begin ();
      i!=noteSpanners.end ();
      i++
    ) {
      S_msrSpanner
        spanner = (*i);

      bool doGenerateSpannerCode = true;

      switch (spanner->getSpannerKind ()) {
        case msrSpanner::kSpannerDashes:
          break;
        case msrSpanner::kSpannerWavyLine:
          if (spanner->getSpannerNoteUpLink ()->getNoteTrillOrnament ()) {
            // don't generate anything, the trill will display the wavy line
            doGenerateSpannerCode = false;
          }
          break;
      } // switch

      if (doGenerateSpannerCode) {
        switch (spanner->getSpannerKind ()) {
          case msrSpanner::kSpannerDashes:
          case msrSpanner::kSpannerWavyLine:
            break;
        } // switch

        generateCodeForSpannerAfterNote (spanner);
      }
    } // for
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrNote& elt)
{
  int inputLineNumber =
    elt->getInputLineNumber ();

#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting note " <<
      elt->asShortString () <<
      ", fOnGoingChord: " << booleanAsString (fOnGoingChord) <<
      ", fOnGoingGraceNotesGroup: " << booleanAsString (fOnGoingGraceNotesGroup) <<
      ", fOnGoingChordGraceNotesGroupLink: " << booleanAsString (fOnGoingChordGraceNotesGroupLink) <<
      ", line " << inputLineNumber <<
      endl;
  }
#endif

  // register the note as on-going right now,
  // since we may return early from this method
  fOnGoingNotesStack.push (elt); // popped in visitEnd (S_msrNote&)

  // is this note to be ignored?
  bool noteIsToBeIgnored = false;

  if (fOnGoingGraceNotesGroup) { // JMI
    if (fOnGoingChord) {
      noteIsToBeIgnored = true;
    }
    else {
      switch (elt->getNoteKind ()) {
        case msrNote::k_NoNoteKind:
          break;

        case msrNote::kRestNote:
          break;

        case msrNote::kSkipNote:
          break;

        case msrNote::kUnpitchedNote:
          break;

        case msrNote::kRegularNote:
          break;

        case msrNote::kDoubleTremoloMemberNote:
          break;

        case msrNote::kGraceNote:
        case msrNote::kGraceSkipNote:
          {
          // don't generate code for the grace notes here, that's done thru
          // the note's noteGraceNotesGroupBefore and noteGraceNotesGroupAfter  fields
            noteIsToBeIgnored = true;
/* JMI
            S_msrNote
              noteTheGraceNotesGroupIsAttachedTo =
                elt->getNoteGraceNotesGroupUpLink ()->
                  getGraceNotesGroupNoteUpLink ();

            // don't generate note if the notes it's grace notes group is attached to
            // has a ??? JMI
            if (
              noteTheGraceNotesGroupIsAttachedTo->getNoteGraceNotesGroupBefore ()
                ||
              noteTheGraceNotesGroupIsAttachedTo->getNoteGraceNotesGroupAfter ()
            ) {
              noteIsToBeIgnored = true;
            }
*/
          }
          break;

        case msrNote::kGraceChordMemberNote:
          break;

        case msrNote::kChordMemberNote:
          break;

        case msrNote::kTupletMemberNote:
          break;

        case msrNote::kTupletRestMemberNote:
          break;

       case msrNote::kGraceTupletMemberNote:
          break;

        case msrNote::kTupletUnpitchedMemberNote:
          break;
      } // switch
    }
  }

  if (fOnGoingRestMeasures) {
    switch (elt->getNoteKind ()) {
      case msrNote::kRestNote:
        // don't handle rest measures, that's done in visitEnd (S_msrRestMeasures&)
          /*
          if (elt->getNoteOccupiesAFullMeasure ()) {
            bool inhibitRestMeasuresBrowsing =
              fVisitedLpsrScore->
                getMsrScore ()->
                  getInhibitRestMeasuresBrowsing ();

            if (inhibitRestMeasuresBrowsing) {
              if (
                gGlobalMsrOahGroup->getTraceMsrVisitors ()
                  ||
                gGlobalTraceOahGroup->getfTraceRestMeasures ()) {
                gLogStream <<
                  "% ==> visiting rest measures is ignored" <<
                  endl;
              }

  #ifdef TRACING_IS_ENABLED
    if (gGlobalTraceOahGroup->getTraceNotesDetails ()) {
      gLogStream <<
        "% ==> returning from visitStart (S_msrNote&)" <<
      endl;
    }
  #endif

            noteIsToBeIgnored = true;
            }
          }
          */

#ifdef TRACING_IS_ENABLED
          if (
            gGlobalMsrOahGroup->getTraceMsrVisitors ()
              ||
            gGlobalTraceOahGroup->getTraceRestMeasures ()
          ) {
            gLogStream <<
              "% ==> start visiting rest notes is ignored upon note " <<
              elt->asString () <<
              endl;
          }
#endif

          noteIsToBeIgnored = true;
        break;

      case msrNote::kSkipNote:
        if (elt->getNoteGraceNotesGroupUpLink ()) {
#ifdef TRACING_IS_ENABLED
          if (
            gGlobalMsrOahGroup->getTraceMsrVisitors ()
              ||
            gGlobalTraceOahGroup->getTraceNotes ()
          ) {
            gLogStream <<
              "% ==> start visiting skip notes is ignored upon note " <<
              elt->asString () <<
              endl;
          }
#endif

          noteIsToBeIgnored = true;
        }
        break;

      case msrNote::kGraceNote:
      case msrNote::kGraceSkipNote:
#ifdef TRACING_IS_ENABLED
          if (
            gGlobalMsrOahGroup->getTraceMsrVisitors ()
              ||
            gGlobalTraceOahGroup->getTraceGraceNotes ()
          ) {
            gLogStream <<
              "% ==> start visiting grace notes is ignored upon note " <<
              elt->asString () <<
              endl;
          }
#endif

          noteIsToBeIgnored = true;
        break;

      case msrNote::kGraceChordMemberNote:
#ifdef TRACING_IS_ENABLED
          if (
            gGlobalMsrOahGroup->getTraceMsrVisitors ()
              ||
            gGlobalTraceOahGroup->getTraceGraceNotes ()
          ) {
            gLogStream <<
              "% ==> start visiting chord grace notes is ignored upon note " <<
              elt->asString () <<
              endl;
          }
#endif

          noteIsToBeIgnored = true;
        break;

      default:
        ;
    } // switch
  }

  // is this note to be ignored?
  if (noteIsToBeIgnored) {
    return;
  }

#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Actually handling note " <<
      elt->asShortString () <<
      ", fOnGoingChord: " << booleanAsString (fOnGoingChord) <<
      ", fOnGoingGraceNotesGroup: " << booleanAsString (fOnGoingGraceNotesGroup) <<
      ", fOnGoingChordGraceNotesGroupLink: " << booleanAsString (fOnGoingChordGraceNotesGroupLink) <<
      ", line " << inputLineNumber <<
      endl;
  }
#endif

  // get the note's grace notes group after
  S_msrGraceNotesGroup
    noteGraceNotesGroupAfter =
      elt->getNoteGraceNotesGroupAfter ();

  // print the note's grace notes group after opener if any
  if (noteGraceNotesGroupAfter) {
    fLilypondCodeStream <<
      "\\afterGrace { ";
  }

  // print the note's grace notes group before if any,
  // unless the note belongs to a chord
  bool doGenerateNoteGraceNotesGroupBefore = true;

  if (fOnGoingChord && fOnGoingGraceNotesGroup) {
    doGenerateNoteGraceNotesGroupBefore = false;
  }

  if (doGenerateNoteGraceNotesGroupBefore) {
    S_msrGraceNotesGroup
      noteGraceNotesGroupBefore =
        elt->getNoteGraceNotesGroupBefore ();

    if (noteGraceNotesGroupBefore) {
      generateGraceNotesGroup (
        noteGraceNotesGroupBefore);
    }
  }

  // print the note scordaturas if any
  const list<S_msrScordatura>&
    noteScordaturas =
      elt->getNoteScordaturas ();

  if (noteScordaturas.size ()) {
    fLilypondCodeStream <<
      " <<" <<
      endl;

    gIndenter++;

    list<S_msrScordatura>::const_iterator
      iBegin = noteScordaturas.begin (),
      iEnd   = noteScordaturas.end (),
      i      = iBegin;
    for ( ; ; ) {
      S_msrScordatura
        scordatura = (*i);

      const list<S_msrStringTuning>&
        scordaturaStringTuningsList =
          scordatura->
            getScordaturaStringTuningsList ();

      gIndenter++;

      fLilypondCodeStream <<
        "\\new Staff \\with { alignAboveContext = \"" <<
        elt->
          fetchNoteStaffUpLink ()->
            getStaffName () <<
        "\" } {" <<
        endl;

      gIndenter++;

      fLilypondCodeStream <<
        "\\hide Staff.Stem" <<
        endl <<
        "\\hide Staff.TimeSignature" <<
        endl <<
        "\\small" <<
        endl <<
        "\\once \\override Score.RehearsalMark.self-alignment-X = #LEFT" <<
        endl <<
        "\\mark\\markup {\\small\\bold \"Scordatura\"}" <<
        endl <<
        "<";

      if (scordaturaStringTuningsList.size ()) {
        list<S_msrStringTuning>::const_iterator
          iBegin = scordaturaStringTuningsList.begin (),
          iEnd   = scordaturaStringTuningsList.end (),
          i      = iBegin;
        for ( ; ; ) {
          S_msrStringTuning
            stringTuning = (*i);

          fLilypondCodeStream <<
            stringTuningAsLilypondString (
              inputLineNumber,
              stringTuning);

          if (++i == iEnd) break;

          fLilypondCodeStream << ' ';
        } // for
      }

      fLilypondCodeStream <<
        ">4" <<
        endl;

      gIndenter--;

      fLilypondCodeStream <<
        "}" <<
        endl;

      gIndenter--;

      fLilypondCodeStream <<
        "{" <<
        endl;

      gIndenter++;

      if (++i == iEnd) break;
    } // for
  }

  // should the note actually be printed?
  msrPrintObjectKind
    notePrintObjectKind =
      elt->getNotePrintObjectKind ();

  if (notePrintObjectKind != fCurrentNotePrinObjectKind) {
    switch (notePrintObjectKind) {
      case kPrintObjectNone:
        // JMI
        break;
      case kPrintObjectYes:
        break;
      case kPrintObjectNo:
        {
          // get the print notes head RGB color atom
          S_oahRGBColorAtom
            nonPrintNotesHeadRGBColorAtom =
              gGlobalLpsr2lilypondOahGroup->
                getNonPrintNotesHeadRGBColorAtom ();

          // has the note color been set?
          if (nonPrintNotesHeadRGBColorAtom->getVariableHasBeenSet ()) {
            const msrRGBColor&
              theRGBColor =
                gGlobalLpsr2lilypondOahGroup->
                  getNonPrintNotesHeadRGBColor ();

            fLilypondCodeStream <<
              endl <<
              "\\once\\override NoteHead.color = #(rgb-color " <<
              theRGBColor.asSpaceSeparatedString (4) <<
              ")" <<
              endl;
          }
          else {
            if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
              fLilypondCodeStream <<
                "%{ " <<
                gGlobalOahOahGroup->getInputSourceName () <<
                ":" <<
                inputLineNumber <<
                ": " <<
                "ignoring 'kPrintObjectNo'" <<
                " %}" <<
                endl;
            }
          }
        }
        break;
    } // switch

    fCurrentNotePrinObjectKind = notePrintObjectKind;
  }

  // print the note slashes if any
  const list<S_msrSlash>&
    noteSlashes =
      elt->getNoteSlashes ();

  if (noteSlashes.size ()) {
    list<S_msrSlash>::const_iterator i;
    for (
      i=noteSlashes.begin ();
      i!=noteSlashes.end ();
      i++
    ) {
      S_msrSlash slash = (*i);

      switch (slash->getSlashTypeKind ()) {
        case k_NoSlashType:
          break;

        case kSlashTypeStart:
          fLilypondCodeStream <<
            endl <<
            "\\override Staff.NoteHead.style = #'slash " <<
            endl;
          break;

        case kSlashTypeStop:
          fLilypondCodeStream <<
            endl <<
            "\\revert Staff.NoteHead.style " <<
            endl;
          break;
      } // switch

      switch (slash->getUseDotsKind ()) {
        case k_NoUseDots:
          break;

        case kUseDotsYes:
          fLilypondCodeStream <<
            endl <<
            "\\override Staff.NoteHead.style = #'slash " <<
            endl;
          break;

        case kUseDotsNo:
          fLilypondCodeStream <<
            endl <<
            "\\revert Staff.NoteHead.style " <<
            endl;
          break;
      } // switch

      switch (slash->getSlashUseStemsKind ()) {
        case k_NoSlashUseStems:
          break;

        case kSlashUseStemsYes:
          fLilypondCodeStream <<
            endl <<
            "\\undo \\hide Staff.Stem " <<
            endl;
          break;

        case kSlashUseStemsNo:
          fLilypondCodeStream <<
            endl <<
            "\\hide Staff.Stem " <<
            endl;
          break;
      } // switch
    } // for
  }

  // print the note wedges circled tips if any
  const list<S_msrWedge>&
    noteWedges =
      elt->getNoteWedges ();

  if (noteWedges.size ()) {
    list<S_msrWedge>::const_iterator i;
    for (
      i=noteWedges.begin ();
      i!=noteWedges.end ();
      i++
    ) {
      S_msrWedge wedge = (*i);

      switch (wedge->getWedgeKind ()) {
        case msrWedge::kWedgeKindNone:
          break;

        case msrWedge::kWedgeCrescendo:
          switch (wedge->getWedgeNienteKind ()) {
            case msrWedge::kWedgeNienteYes:
              fLilypondCodeStream <<
                endl <<
                "\\once\\override Hairpin.circled-tip = ##t " <<
                endl;
              break;
            case msrWedge::kWedgeNienteNo:
              break;
            } // switch
          break;

        case msrWedge::kWedgeDecrescendo:
          switch (wedge->getWedgeNienteKind ()) {
            case msrWedge::kWedgeNienteYes:
              fLilypondCodeStream <<
                endl <<
                "\\once\\override Hairpin.circled-tip = ##t " <<
                endl;
              break;
            case msrWedge::kWedgeNienteNo:
              break;
            } // switch
          break;

        case msrWedge::kWedgeStop:
        /* JMI
          fLilypondCodeStream <<
            "\\! ";
            */
          break;
      } // switch
    } // for
  }

  // print the note slurs line types if any,
  // unless the note is chord member
  if (! elt->getNoteBelongsToAChord ()) {
    const list<S_msrSlur>&
      noteSlurs =
        elt->getNoteSlurs ();

    if (noteSlurs.size ()) {
      list<S_msrSlur>::const_iterator i;
      for (
        i=noteSlurs.begin ();
        i!=noteSlurs.end ();
        i++
      ) {
        S_msrSlur slur = (*i);

        /*
        \slurDashed, \slurDotted, \slurHalfDashed,
        \slurHalfSolid, \slurDashPattern, \slurSolid
        */

        switch (slur->getSlurTypeKind ()) {
          case msrSlur::kRegularSlurStart:
          case msrSlur::kPhrasingSlurStart:
            switch (slur->getSlurLineTypeKind ()) {
              case kLineTypeSolid:
                /* JMI ???
                fLilypondCodeStream <<
                  "\\once\\slurSolid ";
                */
                break;
              case kLineTypeDashed:
                fLilypondCodeStream <<
                  "\\once\\slurDashed ";
                break;
              case kLineTypeDotted:
                fLilypondCodeStream <<
                  "\\once\\slurDotted ";
                break;
              case kLineTypeWavy:
                fLilypondCodeStream <<
                  "\\once\\slurWavy "; // JMI
                break;
            } // switch
            break;
          default:
            ;
        } // switch
      } // for
    }
  }

  // print the note glissandos styles if any
  const list<S_msrGlissando>&
    noteGlissandos =
      elt->getNoteGlissandos ();

  if (noteGlissandos.size ()) {
    list<S_msrGlissando>::const_iterator i;
    for (
      i=noteGlissandos.begin ();
      i!=noteGlissandos.end ();
      i++
    ) {
      S_msrGlissando glissando = (*i);

      switch (glissando->getGlissandoTypeKind ()) {
        case msrGlissando::kGlissandoTypeNone:
          break;

        case msrGlissando::kGlissandoTypeStart:
          // generate the glissando style
          switch (glissando->getGlissandoLineTypeKind ()) {
            case kLineTypeSolid:
              break;
            case kLineTypeDashed:
              fLilypondCodeStream <<
                endl <<
                "\\once\\override Glissando.style = #'dashed-line" <<
                endl;
              break;
            case kLineTypeDotted:
              fLilypondCodeStream <<
                endl <<
                "\\once\\override Glissando.style = #'dotted-line" <<
                endl;
              break;
            case kLineTypeWavy:
              fLilypondCodeStream <<
                endl <<
                "\\once\\override Glissando.style = #'zigzag" <<
                endl;
              break;
          } // switch
          break;

        case msrGlissando::kGlissandoTypeStop:
          break;
      } // switch
    } // for
  }

  // print the note slides styles if any, implemented as glissandos
  const list<S_msrSlide>&
    noteSlides =
      elt->getNoteSlides ();

  if (noteSlides.size ()) {
    list<S_msrSlide>::const_iterator i;
    for (
      i=noteSlides.begin ();
      i!=noteSlides.end ();
      i++
    ) {
      S_msrSlide slide = (*i);

      switch (slide->getSlideTypeKind ()) {
        case msrSlide::kSlideTypeNone:
          break;

        case msrSlide::kSlideTypeStart:
          // generate the glissando style
          switch (slide->getSlideLineTypeKind ()) {
            case kLineTypeSolid:
              break;
            case kLineTypeDashed:
              fLilypondCodeStream <<
                endl <<
                "\\once\\override Glissando.style = #'dashed-line" <<
                endl;
              break;
            case kLineTypeDotted:
              fLilypondCodeStream <<
                endl <<
                "\\once\\override Glissando.style = #'dotted-line" <<
                endl;
              break;
            case kLineTypeWavy:
              fLilypondCodeStream <<
                endl <<
                "\\once\\override Glissando.style = #'zigzag" <<
                endl;
              break;
          } // switch
          break;

        case msrSlide::kSlideTypeStop:
          break;
      } // switch
    } // for
  }

  // print the note glissandos with text if any
  if (noteGlissandos.size ()) {
    list<S_msrGlissando>::const_iterator i;
    for (
      i=noteGlissandos.begin ();
      i!=noteGlissandos.end ();
      i++
    ) {
      S_msrGlissando glissando = (*i);

      switch (glissando->getGlissandoTypeKind ()) {
        case msrGlissando::kGlissandoTypeNone:
          break;

        case msrGlissando::kGlissandoTypeStart:
          {
            string
              glissandoTextValue =
                glissando->getGlissandoTextValue ();

            if (glissandoTextValue.size ()) {
              // generate the glissando text on itself
              fLilypondCodeStream <<
                endl <<
                "\\once\\override Glissando.details.glissando-text = \"" <<
                glissandoTextValue <<
                "\"" <<
                endl <<
                "\\glissandoTextOn" <<
                endl;
            }
          }
          break;

        case msrGlissando::kGlissandoTypeStop:
          break;
      } // switch
    } // for
  }

  // print the note slides with text if any
  if (noteSlides.size ()) {
    list<S_msrSlide>::const_iterator i;
    for (
      i=noteSlides.begin ();
      i!=noteSlides.end ();
      i++
    ) {
      S_msrSlide slide = (*i);

      switch (slide->getSlideTypeKind ()) {
        case msrSlide::kSlideTypeNone:
          break;

        case msrSlide::kSlideTypeStart:
          {
            string
              slideTextValue =
                slide->getSlideTextValue ();

            if (slideTextValue.size ()) {
              // generate the slide text on itself
              fLilypondCodeStream <<
                endl <<
                "\\once\\override Glissando.details.glissando-text = \"" <<
                slideTextValue <<
                "\"" <<
                endl <<
                "\\glissandoTextOn" <<
                endl;
            }
          }
          break;

        case msrSlide::kSlideTypeStop:
          break;
      } // switch
    } // for
  }

  // print the 'before note' spanners if any
//  generateBeforeNoteSpannersIfAny (elt);

  // should the note be parenthesized?
  msrNote::msrNoteHeadParenthesesKind
    noteHeadParenthesesKind =
      elt->getNoteHeadParenthesesKind ();

  switch (noteHeadParenthesesKind) {
    case msrNote::kNoteHeadParenthesesYes:
      fLilypondCodeStream << "\\parenthesize ";
      break;
    case msrNote::kNoteHeadParenthesesNo:
      break;
  } // switch

  // print the note technicals with string if any
  const list<S_msrTechnicalWithString>&
    noteTechnicalWithStrings =
      elt->getNoteTechnicalWithStrings ();

  if (noteTechnicalWithStrings.size ()) {
    list<S_msrTechnicalWithString>::const_iterator i;
    for (
      i=noteTechnicalWithStrings.begin ();
      i!=noteTechnicalWithStrings.end ();
      i++
    ) {
      S_msrTechnicalWithString technicalWithString = (*i);

      switch (technicalWithString->getTechnicalWithStringKind ()) {
        case msrTechnicalWithString::kHammerOn:
          switch (technicalWithString->getTechnicalWithStringTypeKind ()) {
            case kTechnicalTypeStart:
              {
                rational
                  noteSoundingWholeNotes =
                    elt->getNoteSoundingWholeNotes ();

                rational
                  halfWholeNotes =
                    noteSoundingWholeNotes /2;

                fLilypondCodeStream <<
                  "\\after " <<
                  wholeNotesAsLilypondString (
                    inputLineNumber,
                    halfWholeNotes) <<
                  " ^\"H\" ";
              }
              break;
            case kTechnicalTypeStop:
              break;
            case k_NoTechnicalType:
              break;
          } // switch
          break;
        case msrTechnicalWithString::kHandbell: // JMI
          break;
        case msrTechnicalWithString::kOtherTechnical: // JMI
          break;
        case msrTechnicalWithString::kPluck: // JMI
          break;
        case msrTechnicalWithString::kPullOff:
          switch (technicalWithString->getTechnicalWithStringTypeKind ()) {
            case kTechnicalTypeStart:
              {
                rational
                  noteSoundingWholeNotes =
                    elt->getNoteSoundingWholeNotes ();

                rational
                  halfWholeNotes =
                    noteSoundingWholeNotes /2;

                fLilypondCodeStream <<
                  "\\after " <<
                  wholeNotesAsLilypondString (
                    inputLineNumber,
                    halfWholeNotes) <<
                  " ^\"P\" ";
              }
              break;
            case kTechnicalTypeStop:
              break;
            case k_NoTechnicalType:
              break;
          } // switch
          break;
      } // switch
    } // for
  }

  // is the note a cue note?
  switch (elt->getNoteIsACueNoteKind ()) {
    case msrNote::kNoteIsACueNoteYes:
      fLilypondCodeStream <<
        "\\once \\override NoteHead.font-size = -3 ";
      break;
    case msrNote::kNoteIsACueNoteNo:
      break;
  } // switch

  // has the note an octave shift up or down?
  if (! fOnGoingChord) {
    // the octave shift for the chords has already been generated
    S_msrOctaveShift
      noteOctaveShift =
        elt->
          getNoteOctaveShift ();

    if (noteOctaveShift) {
      generateCodeForOctaveShift (
        noteOctaveShift);
    }
  }

  // generate things before the note
  generateBeforeNoteSpannersIfAny (elt);
  generateCodeRightBeforeNote (elt);

  ////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////
  // generate the note itself as a LilyPond string
  ////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////

  generateCodeForNote (elt);

  // generate things after the note
  generateCodeRightAfterNote (elt);
  generateAfterNoteSpannersIfAny (elt);

/* JMI
  // get the note's grace notes group after ??? JMI
  S_msrGraceNotesGroup
    noteGraceNotesGroupAfter =
      elt->getNoteGraceNotesGroupAfter ();

  // print the note's grace notes group after opener if any
  if (noteGraceNotesGroupAfter) {
    fLilypondCodeStream <<
      "\\afterGrace { ";
  }
*/

  if (
    gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()
      ||
    gGlobalLpsr2lilypondOahGroup->getGeneratePositionsInMeasures ()
  ) {
    generateInputLineNumberAndOrPositionInMeasureAsAComment (
      elt);
  }
}

void lpsr2lilypondTranslator::visitEnd (S_msrNote& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting note " <<
      elt->asShortString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  bool noteIsToBeIgnored = false;

  // is this note to be ignored?
  if (fOnGoingChord && fOnGoingGraceNotesGroup) { // JMI
    noteIsToBeIgnored = true;
  }
  if (fOnGoingRestMeasures) {
    switch (elt->getNoteKind ()) {
      case msrNote::kRestNote:
        // don't handle rest measuress, that's done in visitEnd (S_msrRestMeasures&)
          if (elt->getNoteOccupiesAFullMeasure ()) {
            bool inhibitRestMeasuresBrowsing =
              fVisitedLpsrScore->
                getMsrScore ()->
                  getInhibitRestMeasuresBrowsing ();

            if (inhibitRestMeasuresBrowsing) {
#ifdef TRACING_IS_ENABLED
              if (
                gGlobalTraceOahGroup->getTraceNotes ()
                  ||
                gGlobalTraceOahGroup->getTraceRestMeasures ()
              ) {
                gLogStream <<
                  "% ==> end visiting rest measures is ignored" <<
                  endl;
              }
#endif

#ifdef TRACING_IS_ENABLED
              if (gGlobalTraceOahGroup->getTraceNotesDetails ()) {
                gLogStream <<
                  "% ==> returning from visitEnd (S_msrNote&)" <<
                  endl;
              }
#endif

              noteIsToBeIgnored = true;
            }
          }
        break;

      case msrNote::kSkipNote:
        if (elt->getNoteGraceNotesGroupUpLink ()) {
#ifdef TRACING_IS_ENABLED
          if (
            gGlobalMsrOahGroup->getTraceMsrVisitors ()
              ||
            gGlobalTraceOahGroup->getTraceNotes ()
          ) {
            gLogStream <<
              "% ==> end visiting skip notes is ignored" <<
              endl;
          }
#endif

          noteIsToBeIgnored = true;
        }
        break;

      case msrNote::kGraceNote:
      case msrNote::kGraceSkipNote:
#ifdef TRACING_IS_ENABLED
          if (
            gGlobalMsrOahGroup->getTraceMsrVisitors ()
              ||
            gGlobalTraceOahGroup->getTraceGraceNotes ()) {
            gLogStream <<
              "% ==> end visiting grace notes is ignored" <<
              endl;
          }
#endif

          noteIsToBeIgnored = true;
        break;

      default:
        ;
    } // switch
  }

  if (noteIsToBeIgnored) {
    return;
  }

  // fetch the note single tremolo
  S_msrSingleTremolo
    noteSingleTremolo =
      elt->getNoteSingleTremolo ();

  if (noteSingleTremolo) {
    // generate code for the single tremolo only
    // if note doesn't belong to a chord,
    // otherwise it will be generated for the chord itself
    if (! elt->getNoteBelongsToAChord ()) {
      fLilypondCodeStream <<
        singleTremoloDurationAsLilypondString (
          noteSingleTremolo);
    }
  }

  // print the note words if any,
  // which should precede the articulations in LilyPond
  // generate code for the words only
  // if note doesn't belong to a chord,
  // otherwise it will be generated for the chord itself
  if (! elt->getNoteBelongsToAChord ()) {
    const list<S_msrWords>&
      noteWords =
        elt->getNoteWords ();

    if (noteWords.size ()) {
      list<S_msrWords>::const_iterator i;
      for (
        i=noteWords.begin ();
        i!=noteWords.end ();
        i++
      ) {
        msrPlacementKind
          wordsPlacementKind =
            (*i)->getWordsPlacementKind ();

        string wordsContents =
          (*i)->getWordsContents ();

        msrFontStyleKind
          wordsFontStyleKind =
            (*i)->getWordsFontStyleKind ();

        S_msrFontSize
          wordsFontSize =
            (*i)->getWordsFontSize ();

        msrFontWeightKind
          wordsFontWeightKind =
            (*i)->getWordsFontWeightKind ();

        string markup;

        {
          // create markup apart to have its length available
          stringstream s;

          switch (wordsPlacementKind) {
            case kPlacementNone:
              s << "^";
              break;
            case kPlacementAbove:
              s << "^";
              break;
            case kPlacementBelow:
              s << "_";
              break;
          } // switch

          s <<
            "\\markup" << " { ";

          switch (wordsFontStyleKind) {
            case kFontStyleNone:
              break;
            case kFontStyleNormal:
              // LilyPond produces 'normal style' text by default
              break;
            case KFontStyleItalic:
              s <<
                "\\italic ";
              break;
          } // switch

          switch (wordsFontWeightKind) {
            case kFontWeightNone:
              break;
            case kFontWeightNormal:
              // LilyPond produces 'normal weight' text by default
              break;
            case kFontWeightBold:
              s <<
                "\\bold ";
              break;
          } // switch

          switch (wordsFontSize->getFontSizeKind ()) {
            case msrFontSize::kFontSizeNone:
              break;
            case msrFontSize::kFontSizeXXSmall:
              s <<
                "\\tiny ";
              break;
            case msrFontSize::kFontSizeXSmall:
              s <<
                "\\smaller ";
              break;
            case msrFontSize::kFontSizeSmall:
              s <<
                "\\small ";
              break;
            case msrFontSize::kFontSizeMedium:
              s <<
                "\\normalsize ";
              break;
            case msrFontSize::kFontSizeLarge:
              s <<
                "\\large ";
              break;
            case msrFontSize::kFontSizeXLarge:
              s <<
                "\\larger ";
              break;
            case msrFontSize::kFontSizeXXLarge:
              s <<
                "\\huge ";
              break;
            case msrFontSize::kFontSizeNumeric:
            /* JMI
              s <<
                "%{ " <<
                wordsFontSize->getFontNumericSize () <<
                " points %} ";
                */
              break;
          } // switch

          // does wordsContents contains end of lines?
          size_t endOfLineFound = wordsContents.find ("\n");

          if (endOfLineFound == string::npos) {
            s <<
     // JMI         doubleQuoteStringIfNonAlpha (wordsContents) <<
              "\"" << wordsContents << "\"";
            }
          else {
            s <<
              gIndenter.indentMultiLineString (
                generateAColumnForMarkup (
                  wordsContents,
                  markupColumnKindLeftAligned));
          }

          s <<
            " } ";

          markup = s.str ();
        }

        fLilypondCodeStream <<
          markup;
      } // for
    }
  }

/* TOO EARLY FOR ALL OF THEM??? JMI
  // print the note articulations if any
  if (! fOnGoingChord) {
    const list<S_msrArticulation>&
      noteArticulations =
        elt->getNoteArticulations ();

    if (noteArticulations.size ()) {
      list<S_msrArticulation>::const_iterator i;
      for (
        i=noteArticulations.begin ();
        i!=noteArticulations.end ();
        i++
      ) {
        S_msrArticulation articulation = (*i);
        switch (articulation->getArticulationKind ()) {
          case msrArticulation::kFermata: // handle this better JMI
            if (
              // fermata?
              S_msrFermata
                fermata =
                  dynamic_cast<msrFermata*>(&(*articulation))
              ) {
              switch (fermata->getFermataTypeKind ()) {
                case msrFermata::kFermataTypeNone:
                  // no placement needed
                  break;
                case msrFermata::kFermataTypeUpright:
                  // no placement needed
                  break;
                case msrFermata::kFermataTypeInverted:
                  fLilypondCodeStream << "_";
                  break;
              } // switch

              switch (fermata->getFermataKind ()) {
                case msrFermata::kNormalFermataKind:
                  fLilypondCodeStream << "\\fermata ";
                  break;
                case msrFermata::kAngledFermataKind:
                  fLilypondCodeStream << "\\shortfermata ";
                  break;
                case msrFermata::kSquareFermataKind:
                  fLilypondCodeStream << "\\longfermata ";
                  break;
              } // switch
            }
            else {
              stringstream s;

              s <<
                "note articulation '" <<
                articulation->asString () <<
                "' has 'fermata' kind, but is not of type S_msrFermata" <<
                ", line " << articulation->getInputLineNumber ();

              msrInternalError (
                gGlobalOahOahGroup->getInputSourceName (),
                articulation->getInputLineNumber (),
                __FILE__, __LINE__,
                s.str ());
            }
            break;

          default:
            generateNoteArticulation ((*i));
            fLilypondCodeStream <<
              ' ';
        } // switch
      } // for
    }
  }
  */

  // print the note technicals if any
  const list<S_msrTechnical>&
    noteTechnicals =
      elt->getNoteTechnicals ();

  if (noteTechnicals.size ()) {
    list<S_msrTechnical>::const_iterator i;
    for (
      i=noteTechnicals.begin ();
      i!=noteTechnicals.end ();
      i++
    ) {

      fLilypondCodeStream <<
        technicalAsLilypondString ((*i));

      switch ((*i)->getTechnicalPlacementKind ()) {
        case kPlacementNone:
          break;
        case kPlacementAbove:
          fLilypondCodeStream << "^";
          break;
        case kPlacementBelow:
          fLilypondCodeStream << "_";
          break;
      } // switch

      fLilypondCodeStream << ' ';
    } // for
  }

  // print the note technicals with integer if any,
  // but not for chord member notes strings:
  // they should appear after the chord itself
  switch (elt->getNoteKind ()) {
    case msrNote::kChordMemberNote:
       break;

    default:
      {
        const list<S_msrTechnicalWithInteger>&
          noteTechnicalWithIntegers =
            elt->getNoteTechnicalWithIntegers ();

        if (noteTechnicalWithIntegers.size ()) {
          list<S_msrTechnicalWithInteger>::const_iterator i;
          for (
            i=noteTechnicalWithIntegers.begin ();
            i!=noteTechnicalWithIntegers.end ();
            i++
          ) {

            S_msrTechnicalWithInteger
                technicalWithInteger = (*i);

            fLilypondCodeStream <<
              technicalWithIntegerAsLilypondString (
                technicalWithInteger);

            switch (technicalWithInteger->getTechnicalWithIntegerPlacementKind ()) {
              case kPlacementNone:
                break;
              case kPlacementAbove:
                fLilypondCodeStream << "^";
                break;
              case kPlacementBelow:
                fLilypondCodeStream << "_";
                break;
            } // switch

            fLilypondCodeStream << ' ';
          } // for
        }
      }
  } // switch

  // print the note technicals with float if any,
  // but not for chord member notes strings:
  // they should appear after the chord itself
  switch (elt->getNoteKind ()) {
    case msrNote::kChordMemberNote:
       break;

    default:
      {
        const list<S_msrTechnicalWithFloat>&
          noteTechnicalWithFloats =
            elt->getNoteTechnicalWithFloats ();

        if (noteTechnicalWithFloats.size ()) {
          list<S_msrTechnicalWithFloat>::const_iterator i;
          for (
            i=noteTechnicalWithFloats.begin ();
            i!=noteTechnicalWithFloats.end ();
            i++
          ) {

            S_msrTechnicalWithFloat
                technicalWithFloat = (*i);

            fLilypondCodeStream <<
              technicalWithFloatAsLilypondString (
                technicalWithFloat);

            switch (technicalWithFloat->getTechnicalWithFloatPlacementKind ()) {
              case kPlacementNone:
                break;
              case kPlacementAbove:
                fLilypondCodeStream << "^";
                break;
              case kPlacementBelow:
                fLilypondCodeStream << "_";
                break;
            } // switch

            fLilypondCodeStream << ' ';
          } // for
        }
      }
  } // switch

  // print the note technicals with string if any
  const list<S_msrTechnicalWithString>&
    noteTechnicalWithStrings =
      elt->getNoteTechnicalWithStrings ();

  if (noteTechnicalWithStrings.size ()) {
    list<S_msrTechnicalWithString>::const_iterator i;
    for (
      i=noteTechnicalWithStrings.begin ();
      i!=noteTechnicalWithStrings.end ();
      i++
    ) {

      fLilypondCodeStream <<
        technicalWithStringAsLilypondString ((*i));

      switch ((*i)->getTechnicalWithStringPlacementKind ()) {
        case kPlacementNone:
          break;
        case kPlacementAbove:
          fLilypondCodeStream << "^";
          break;
        case kPlacementBelow:
          fLilypondCodeStream << "_";
          break;
      } // switch

      fLilypondCodeStream << ' ';
    } // for
  }

  // print the note ornaments if any
  list<S_msrOrnament>
    noteOrnaments =
      elt->getNoteOrnaments ();

  if (noteOrnaments.size ()) {
    list<S_msrOrnament>::const_iterator i;
    for (
      i=noteOrnaments.begin ();
      i!=noteOrnaments.end ();
      i++
    ) {
      S_msrOrnament
        ornament = (*i);

      generateOrnament (ornament); // some ornaments are not yet supported
    } // for
  }

  // print the note dynamics if any
  if (! fOnGoingChord) {
    const list<S_msrDynamics>&
      noteDynamics =
        elt->getNoteDynamics ();

    if (noteDynamics.size ()) {
      list<S_msrDynamics>::const_iterator i;
      for (
        i=noteDynamics.begin ();
        i!=noteDynamics.end ();
        i++
      ) {
        S_msrDynamics
          dynamics = (*i);

        switch (dynamics->getDynamicsPlacementKind ()) {
          case kPlacementNone:
     // JMI       fLilypondCodeStream << "-3";
            break;
          case kPlacementAbove:
            fLilypondCodeStream << "^";
            break;
          case kPlacementBelow:
            // this is done by LilyPond by default
            break;
        } // switch

        fLilypondCodeStream <<
          dynamicsAsLilypondString (dynamics) << ' ';
      } // for
    }
  }

  // print the note other dynamics if any
  if (! fOnGoingChord) {
    const list<S_msrOtherDynamics>&
      noteOtherDynamics =
        elt->getNoteOtherDynamics ();

    if (noteOtherDynamics.size ()) {
      list<S_msrOtherDynamics>::const_iterator i;
      for (
        i=noteOtherDynamics.begin ();
        i!=noteOtherDynamics.end ();
        i++
      ) {
        S_msrOtherDynamics
          otherDynamics = (*i);

        switch (otherDynamics->getOtherDynamicsPlacementKind ()) {
          case kPlacementNone:
            fLilypondCodeStream << "-";
            break;
          case kPlacementAbove:
            fLilypondCodeStream << "^";
            break;
          case kPlacementBelow:
            // this is done by LilyPond by default
            break;
        } // switch

        fLilypondCodeStream <<
          "\\otherDynamic \"" <<
          otherDynamics->getOtherDynamicsString () <<
          "\" ";
      } // for
    }
  }

  // print the note beams if any,
  // unless the note is chord member
  bool doGenerateBeams = true;
  if (elt->getNoteBelongsToAChord ()) {
    doGenerateBeams = false;
  }
  if (doGenerateBeams) {
    generateNoteBeams (elt);
  }

  // print the note slurs if any,
  // unless the note is chord member
  bool doGenerateSlurs = true;
  if (elt->getNoteBelongsToAChord ()) {
     doGenerateSlurs = false;
  }
  if (doGenerateSlurs) {
    generateNoteSlurs (elt);
  }

  // print the note ligatures if any
  const list<S_msrLigature>&
    noteLigatures =
      elt->getNoteLigatures ();

  if (noteLigatures.size ()) {
    list<S_msrLigature>::const_iterator i;
    for (
      i=noteLigatures.begin ();
      i!=noteLigatures.end ();
      i++
    ) {
      switch ((*i)->getLigatureKind ()) {
        case msrLigature::kLigatureNone:
          break;
        case msrLigature::kLigatureStart:
   // JMI       fLilypondCodeStream << "\\[ ";
          break;
        case msrLigature::kLigatureContinue:
          break;
        case msrLigature::kLigatureStop:
          fLilypondCodeStream << "\\] ";
          break;
      } // switch
    } // for
  }

  // print the note wedges if any
  const list<S_msrWedge>&
    noteWedges =
      elt->getNoteWedges ();

  if (noteWedges.size ()) {
    list<S_msrWedge>::const_iterator i;
    for (
      i=noteWedges.begin ();
      i!=noteWedges.end ();
      i++
    ) {
      S_msrWedge wedge = (*i);

      switch (wedge->getWedgeKind ()) {
        case msrWedge::kWedgeKindNone:
          break;

        case msrWedge::kWedgeCrescendo:
          switch (wedge->getWedgePlacementKind ()) {
            case kPlacementNone:
              break;
            case kPlacementAbove:
              fLilypondCodeStream <<
                "^";
              break;
            case kPlacementBelow:
              fLilypondCodeStream <<
                "_";
              break;
            } // switch
          fLilypondCodeStream <<
            "\\< ";
          break;

        case msrWedge::kWedgeDecrescendo:
          switch (wedge->getWedgePlacementKind ()) {
            case kPlacementNone:
              break;
            case kPlacementAbove:
              fLilypondCodeStream <<
                "^";
              break;
            case kPlacementBelow:
              fLilypondCodeStream <<
                "_";
              break;
            } // switch
          fLilypondCodeStream <<
            "\\> ";
          break;

        case msrWedge::kWedgeStop:
          fLilypondCodeStream <<
            "\\! ";
          break;
      } // switch
    } // for
  }

  // print the note articulations if any,
  // which should follo the dynamics and wedges in LilyPond
  if (! fOnGoingChord) {
    const list<S_msrArticulation>&
      noteArticulations =
        elt->getNoteArticulations ();

    if (noteArticulations.size ()) {
      list<S_msrArticulation>::const_iterator i;
      for (
        i=noteArticulations.begin ();
        i!=noteArticulations.end ();
        i++
      ) {
        S_msrArticulation articulation = (*i);

        switch (articulation->getArticulationKind ()) {
          case msrArticulation::kFermata: // handle this better JMI
            if (
              // fermata?
              S_msrFermata
                fermata =
                  dynamic_cast<msrFermata*>(&(*articulation))
              ) {
              switch (fermata->getFermataTypeKind ()) {
                case msrFermata::kFermataTypeNone:
                  // no placement needed
                  break;
                case msrFermata::kFermataTypeUpright:
                  // no placement needed
                  break;
                case msrFermata::kFermataTypeInverted:
                  fLilypondCodeStream << "_";
                  break;
              } // switch

              switch (fermata->getFermataKind ()) {
                case msrFermata::kNormalFermataKind:
                  if (
                    elt->getNoteOccupiesAFullMeasure ()
                      &&
                    elt->getNoteIsARest ()
                  ) {
                    fLilypondCodeStream << "\\fermataMarkup ";
                  }
                  else {
                    fLilypondCodeStream << "\\fermata ";
                  }
                  break;
                case msrFermata::kAngledFermataKind:
                  fLilypondCodeStream << "\\shortfermata ";
                  break;
                case msrFermata::kSquareFermataKind:
                  fLilypondCodeStream << "\\longfermata ";
                  break;
              } // switch
            }
            else {
              stringstream s;

              s <<
                "note articulation '" <<
                articulation->asString () <<
                "' has 'fermata' kind, but is not of type S_msrFermata" <<
                ", line " << articulation->getInputLineNumber ();

              msrInternalError (
                gGlobalOahOahGroup->getInputSourceName (),
                articulation->getInputLineNumber (),
                __FILE__, __LINE__,
                s.str ());
            }
            break;

          default:
            generateNoteArticulation ((*i));
            fLilypondCodeStream <<
              " ";
        } // switch
      } // for
    }
  }

  // print the note glissandos if any
  const list<S_msrGlissando>&
    noteGlissandos =
      elt->getNoteGlissandos ();

  if (noteGlissandos.size ()) {
    list<S_msrGlissando>::const_iterator i;
    for (
      i=noteGlissandos.begin ();
      i!=noteGlissandos.end ();
      i++
    ) {
      S_msrGlissando glissando = (*i);

      switch (glissando->getGlissandoTypeKind ()) {
        case msrGlissando::kGlissandoTypeNone:
          break;

        case msrGlissando::kGlissandoTypeStart:
          // generate the glissando itself
          fLilypondCodeStream <<
            "\\glissando " <<
            "\\glissandoTextOff ";
          break;

        case msrGlissando::kGlissandoTypeStop:
          break;
      } // switch
    } // for
  }

  // print the note slides if any, implemented as glissandos
  const list<S_msrSlide>&
    noteSlides =
      elt->getNoteSlides ();

  if (noteSlides.size ()) {
    list<S_msrSlide>::const_iterator i;
    for (
      i=noteSlides.begin ();
      i!=noteSlides.end ();
      i++
    ) {
      S_msrSlide slide = (*i);

      switch (slide->getSlideTypeKind ()) {
        case msrSlide::kSlideTypeNone:
          break;

        case msrSlide::kSlideTypeStart:
          // generate the glissando itself
          fLilypondCodeStream <<
            "\\glissando " <<
            "\\glissandoTextOff ";
          break;

        case msrSlide::kSlideTypeStop:
          break;
      } // switch
    } // for
  }

  // print the note spanners if any
  const list<S_msrSpanner>&
    noteSpanners =
      elt->getNoteSpanners ();

  if (noteSpanners.size ()) {
    list<S_msrSpanner>::const_iterator i;
    for (
      i=noteSpanners.begin ();
      i!=noteSpanners.end ();
      i++
    ) {
      S_msrSpanner
        spanner = (*i);

      bool doGenerateSpannerCode = true;

      switch (spanner->getSpannerKind ()) {
        case msrSpanner::kSpannerDashes:
          break;
        case msrSpanner::kSpannerWavyLine:
          if (spanner->getSpannerNoteUpLink ()->getNoteTrillOrnament ()) {
            // don't generate anything, the trill will display the wavy line
            doGenerateSpannerCode = false;
          }
          break;
      } // switch

      if (doGenerateSpannerCode) {
        switch (spanner->getSpannerPlacementKind ()) {
          case kPlacementNone:
     // JMI       fLilypondCodeStream << "-3";
            break;
          case kPlacementAbove:
            fLilypondCodeStream << "^";
            break;
          case kPlacementBelow:
            // this is done by LilyPond by default
            break;
        } // switch

        generateCodeForSpannerAfterNote (spanner);
      }
    } // for
  }

  // are there note scordaturas?
  const list<S_msrScordatura>&
    noteScordaturas =
      elt->getNoteScordaturas ();

  if (noteScordaturas.size ()) { // g_Indenter ??? JMI
    gIndenter--;

    fLilypondCodeStream <<
      endl <<
      "}" <<
      endl;

    gIndenter--;

    fLilypondCodeStream <<
      ">>" <<
      endl;
  }

  if (elt->getNoteIsFollowedByGraceNotesGroup ()) { // JMI
    if (! elt->getNoteIsARest ()) {
      fLilypondCodeStream <<
       " % noteIsFollowedByGraceNotesGroup" <<
        endl; // JMI ???
    }
  }

  // get the note's grace notes now,
  // unless the note belongs to a grace notes group link
  if (! fOnGoingChordGraceNotesGroupLink) {
    S_msrGraceNotesGroup
      noteGraceNotesGroupAfter =
        elt->getNoteGraceNotesGroupAfter ();

    // print the note's grace notes after group closer if any
    if (noteGraceNotesGroupAfter) {
      fLilypondCodeStream <<
        "} ";
      generateGraceNotesGroup (
        noteGraceNotesGroupAfter);
    }
  }

  fOnGoingNotesStack.pop (); // was pushed in visitStart (S_msrNote&)
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrOctaveShift& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrOctaveShift" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrOctaveShift& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrOctaveShift" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrAccordionRegistration& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrAccordionRegistration" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  int highDotsNumber =
    elt->getHighDotsNumber ();
  int middleDotsNumber =
    elt->getMiddleDotsNumber ();
  int lowDotsNumber =
    elt->getLowDotsNumber ();

  string numbersToBeUsed;
  bool   nonZeroNumberHasBeenIssued = false;

  // the numbers should be written in the order 'high, middle, low'
  // and 0 ahead of the specification is forbidden

  if (highDotsNumber > 0) {
    numbersToBeUsed +=
      to_string (highDotsNumber);
    nonZeroNumberHasBeenIssued = true;
  }

  if (middleDotsNumber > 0) {
    numbersToBeUsed +=
      to_string (middleDotsNumber);
    nonZeroNumberHasBeenIssued = true;
  }
  else {
    if (nonZeroNumberHasBeenIssued) {
      numbersToBeUsed +=
        to_string (middleDotsNumber);
    }
  }

  numbersToBeUsed +=
    to_string (lowDotsNumber);

  fLilypondCodeStream <<
    "\\discant \"" <<
    numbersToBeUsed <<
    "\" ";
}

void lpsr2lilypondTranslator::visitStart (S_msrHarpPedalsTuning& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrHarpPedalsTuning" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  map<msrDiatonicPitchKind, msrAlterationKind>
    harpPedalsAlterationKindsMap =
      elt->getHarpPedalsAlterationKindsMap ();

  if (harpPedalsAlterationKindsMap.size ()) {
    gIndenter++;

    fLilypondCodeStream <<
      "_\\markup { \\harp-pedal #\"" <<
      harpPedalTuningAsLilypondString (
        harpPedalsAlterationKindsMap [kD]) <<
      harpPedalTuningAsLilypondString (
        harpPedalsAlterationKindsMap [kC]) <<
      harpPedalTuningAsLilypondString (
        harpPedalsAlterationKindsMap [kB]) <<
      "|" <<
      harpPedalTuningAsLilypondString (
        harpPedalsAlterationKindsMap [kE]) <<
      harpPedalTuningAsLilypondString (
        harpPedalsAlterationKindsMap [kF]) <<
      harpPedalTuningAsLilypondString (
        harpPedalsAlterationKindsMap [kG]) <<
      harpPedalTuningAsLilypondString (
        harpPedalsAlterationKindsMap [kA]) <<
      "\" } " <<
      endl;

    gIndenter--;
  }
  else {
    fLilypondCodeStream <<
      "%{empty harp pedals tuning???%} "; // JMI
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrStem& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrStem" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrStem& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrStem" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrBeam& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrBeam" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrBeam& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrBeam" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generateCodeForOctaveShift (
  S_msrOctaveShift octaveShift)
{
  msrOctaveShift::msrOctaveShiftKind
    octaveShiftKind =
      octaveShift->
        getOctaveShiftKind ();

  int
    octaveShiftSize =
      octaveShift->
        getOctaveShiftSize ();

  switch (octaveShiftKind) {
    case msrOctaveShift::kOctaveShiftNone:
      break;
    case msrOctaveShift::kOctaveShiftUp:
      fLilypondCodeStream <<
      "\\ottava #" <<
        "-" << (octaveShiftSize - 1) / 7 << // 1 or 2
        ' ';
      break;
    case msrOctaveShift::kOctaveShiftDown:
      fLilypondCodeStream <<
        "\\ottava #" <<
        (octaveShiftSize - 1) / 7 << // 1 or 2
        ' ';
      break;
    case msrOctaveShift::kOctaveShiftStop:
          fLilypondCodeStream <<
            "\\ottava #0 ";
      break;
    case msrOctaveShift::kOctaveShiftContinue:
      break;
  } // switch
}

//________________________________________________________________________
void lpsr2lilypondTranslator::generateCodeAHeadOfChordContents (
  S_msrChord chord)
{
  // generate the chord codas if any
  const list<S_msrCoda>&
    chordCodas =
      chord->getChordCodas ();

  if (chordCodas.size ()) {
    list<S_msrCoda>::const_iterator i;
    for (i=chordCodas.begin (); i!=chordCodas.end (); i++) {
      // generate the coda
      S_msrCoda coda = (*i);

      generateCoda (coda);
    } // for
  }

  // generate the chord segnos if any
  const list<S_msrSegno>&
    chordSegnos =
      chord->getChordSegnos ();

  if (chordSegnos.size ()) {
    list<S_msrSegno>::const_iterator i;
    for (i=chordSegnos.begin (); i!=chordSegnos.end (); i++) {
      // generate the segno
      generateSegno ((*i));
    } // for
  }
}

void lpsr2lilypondTranslator::generateCodeRightBeforeChordContents (
  S_msrChord chord)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceChords ()) {
    fLilypondCodeStream <<
      "% --> generateCodeRightBeforeChordContents() for chord " <<
      chord->asShortString () <<
      ", line " << chord->getInputLineNumber () <<
      endl;
  }
#endif

/* JMI
  gLogStream <<
    "% chordGraceNotesGroupBefore = ";
  if (chordGraceNotesGroupBefore) {
    gLogStream <<
      chordGraceNotesGroupBefore;
  }
  else {
    gLogStream <<
      "nullptr";
  }
  gLogStream << endl;
*/

  // print the chord's grace notes before if any,
  S_msrChordGraceNotesGroupLink
    chordGraceNotesGroupLinkBefore =
      chord->getChordGraceNotesGroupLinkBefore ();

  if (chordGraceNotesGroupLinkBefore) {
    generateGraceNotesGroup (
      chordGraceNotesGroupLinkBefore->
        getOriginalGraceNotesGroup ());
  }

  // get the chord glissandos
  const list<S_msrGlissando>&
    chordGlissandos =
      chord->getChordGlissandos ();

  // print the chord glissandos styles if any
  if (chordGlissandos.size ()) {
    list<S_msrGlissando>::const_iterator i;
    for (
      i=chordGlissandos.begin ();
      i!=chordGlissandos.end ();
      i++
    ) {
      S_msrGlissando glissando = (*i);

      switch (glissando->getGlissandoTypeKind ()) {
        case msrGlissando::kGlissandoTypeNone:
          break;

        case msrGlissando::kGlissandoTypeStart:
          // generate the glissando style
          switch (glissando->getGlissandoLineTypeKind ()) {
            case kLineTypeSolid:
              break;
            case kLineTypeDashed:
              fLilypondCodeStream <<
                "\\once\\override Glissando.style = #'dashed-line ";
              break;
            case kLineTypeDotted:
              fLilypondCodeStream <<
                "\\once\\override Glissando.style = #'dotted-line ";
              break;
            case kLineTypeWavy:
              fLilypondCodeStream <<
                "\\once\\override Glissando.style = #'zigzag ";
              break;
          } // switch
          break;

        case msrGlissando::kGlissandoTypeStop:
          break;
      } // switch
    } // for
  }

  // get the chord slides
  const list<S_msrSlide>&
    chordSlides =
      chord->getChordSlides ();

  // print the chord slides styles if any, implemented as glissandos
  if (chordSlides.size ()) {
    list<S_msrSlide>::const_iterator i;
    for (
      i=chordSlides.begin ();
      i!=chordSlides.end ();
      i++
    ) {
      S_msrSlide slide = (*i);

      switch (slide->getSlideTypeKind ()) {
        case msrSlide::kSlideTypeNone:
          break;

        case msrSlide::kSlideTypeStart:
          // generate the glissando style
          switch (slide->getSlideLineTypeKind ()) {
            case kLineTypeSolid:
              break;
            case kLineTypeDashed:
              fLilypondCodeStream <<
                "\\once\\override Glissando.style = #'dashed-line ";
              break;
            case kLineTypeDotted:
              fLilypondCodeStream <<
                "\\once\\override Glissando.style = #'dotted-line ";
              break;
            case kLineTypeWavy:
              fLilypondCodeStream <<
                "\\once\\override Glissando.style = #'zigzag ";
              break;
          } // switch
          break;

        case msrSlide::kSlideTypeStop:
          break;
      } // switch
    } // for
  }

  // get the chord ligatures
  list<S_msrLigature>
    chordLigatures =
      chord->getChordLigatures ();

  // print the chord ligatures if any
  if (chordLigatures.size ()) {
    list<S_msrLigature>::const_iterator i;
    for (
      i=chordLigatures.begin ();
      i!=chordLigatures.end ();
      i++
    ) {

      switch ((*i)->getLigatureKind ()) {
        case msrLigature::kLigatureNone:
          break;
        case msrLigature::kLigatureStart:
          fLilypondCodeStream << "\\[ ";
          break;
        case msrLigature::kLigatureContinue:
          break;
        case msrLigature::kLigatureStop:
  // JMI        fLilypondCodeStream << "\\] ";
          break;
      } // switch
    } // for
  }

  // don't take the chord into account for line breaking ??? JMI

  // get the chord articulations
  list<S_msrArticulation>
    chordArticulations =
      chord->getChordArticulations ();

  // print the chord arpeggios directions if any
  if (chordArticulations.size ()) {
    list<S_msrArticulation>::const_iterator i;
    for (
      i=chordArticulations.begin ();
      i!=chordArticulations.end ();
      i++
    ) {
      S_msrArticulation articulation = (*i);

      if (
        // arpeggiato?
        S_msrArpeggiato
          arpeggiato =
            dynamic_cast<msrArpeggiato*>(&(*articulation))
        ) {
        msrDirectionKind
          directionKind =
            arpeggiato->getArpeggiatoDirectionKind ();

        switch (directionKind) {
          case kDirectionNone:
            fLilypondCodeStream <<
              endl <<
              "\\arpeggioNormal";
            break;
          case kDirectionUp:
            fLilypondCodeStream <<
              endl <<
              "\\arpeggioArrowUp";
            break;
          case kDirectionDown:
            fLilypondCodeStream <<
              endl <<
              "\\arpeggioArrowDown";
            break;
        } // switch

        fLilypondCodeStream << ' ';

        fCurrentArpeggioDirectionKind = directionKind;
      }

      else if (
        // non arpeggiato?
        S_msrNonArpeggiato
          nonArpeggiato =
            dynamic_cast<msrNonArpeggiato*>(&(*articulation))
        ) {
        fLilypondCodeStream <<
          endl <<
          "\\arpeggioBracket";

        switch (nonArpeggiato->getNonArpeggiatoTypeKind ()) {
          case msrNonArpeggiato::kNonArpeggiatoTypeNone:
            fLilypondCodeStream << " %{\\kNonArpeggiatoTypeNone???%}";
            break;
          case msrNonArpeggiato::kNonArpeggiatoTypeTop:
            fLilypondCodeStream << " %{\\kNonArpeggiatoTypeTop???%}";
            break;
          case msrNonArpeggiato::kNonArpeggiatoTypeBottom:
            fLilypondCodeStream << " %{\\kNonArpeggiatoTypeBottom???%}";
            break;
        } // switch

        fLilypondCodeStream << endl;
      }
   } // for
  }

  // should a stem direction command be generated?
  const list<S_msrStem>&
    chordStems =
      chord->getChordStems ();

  if (chordStems.size ()) {
    list<S_msrStem>::const_iterator
      iBegin = chordStems.begin ();

    // only the first stem is used, since they all have the same kind
    S_msrStem firstChordStem = (*iBegin);

    generateNoteStemIfNeeded (firstChordStem);

    fLilypondCodeStream <<
      ' ';
  }

  // should an octave shift be generated?
  S_msrOctaveShift
    chordOctaveShift =
      chord->getChordOctaveShift ();

  if (chordOctaveShift) {
    generateCodeForOctaveShift (
      chordOctaveShift);
  }
}

void lpsr2lilypondTranslator::generateCodeForChordInGraceNotesGroupContents (
  S_msrChord chord)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceChords ()) {
    fLilypondCodeStream <<
      "% --> generateCodeForChordInGraceNotesGroupContents() for chord " <<
      chord->asShortString () <<
      ", line " << chord->getInputLineNumber () <<
      endl;
  }
#endif

  // get the chord notes vector
  const vector<S_msrNote>&
    chordNotesVector =
      chord->getChordNotesVector ();

  // generate the chord notes KOF JMI
  if (chordNotesVector.size ()) {
    vector<S_msrNote>::const_iterator
      iBegin = chordNotesVector.begin (),
      iEnd   = chordNotesVector.end (),
      i      = iBegin;
    for ( ; ; ) {
      S_msrNote
        chordNote = (*i);

      // print things before the chordNote
      generateCodeRightBeforeNote (chordNote);

      // print the chordNote itself
      generateCodeForNote (chordNote);

      // print things after the chordNote
      generateCodeRightAfterNote (chordNote);

      if (++i == iEnd) break;
      fLilypondCodeStream <<
        ' ';
    } // for
  }
}

void lpsr2lilypondTranslator::generateCodeRightAfterChordContents (
  S_msrChord chord)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceChords ()) {
    fLilypondCodeStream <<
      endl <<
      "% --> generateCodeRightAfterChordContents() for chord " <<
      chord->asShortString () <<
      ", line " << chord->getInputLineNumber () <<
      endl;
  }
#endif

  // get the chord notes vector
  const vector<S_msrNote>&
    chordNotesVector =
      chord->getChordNotesVector ();

  // if the preceding item is a chord, the first note of the chord
  // is used as the reference point for the octave placement
  // of a following note or chord
  switch (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) {
    case kOctaveEntryRelative:
      fCurrentOctaveEntryReference =
        chordNotesVector [0];
      break;
    case kOctaveEntryAbsolute:
      break;
    case kOctaveEntryFixed:
      break;
  } // switch

  // generate the chord duration if relevant
  if (
    chord->getChordIsFirstChordInADoubleTremolo ()
      ||
    chord->getChordIsSecondChordInADoubleTremolo ()) {
      // print chord duration
      fLilypondCodeStream <<
        chord->getChordSoundingWholeNotes ();
  }

  else {
    int chordInputLineNumber =
      chord->getInputLineNumber ();

    // print the chord duration
    fLilypondCodeStream <<
      durationAsLilypondString (
        chordInputLineNumber,
        chord->
          getChordDisplayWholeNotes ()); // JMI test wether chord is in a tuplet?
  }

/* JMI
  gLogStream <<
    "% chordGraceNotesGroupAfter = ";
  if (chordGraceNotesGroupAfter) {
    gLogStream <<
      chordGraceNotesGroupAfter;
  }
  else {
    gLogStream <<
      "nullptr";
  }
  gLogStream << endl;

  if (chordGraceNotesGroupAfter) {
    generateGraceNotesGroup (
      chordGraceNotesGroupAfter);
  }
*/

  // print the chord's grace notes after if any
  S_msrChordGraceNotesGroupLink
    chordGraceNotesGroupLinkAfter =
      chord->getChordGraceNotesGroupLinkAfter ();

  if (chordGraceNotesGroupLinkAfter) {
    generateGraceNotesGroup (
      chordGraceNotesGroupLinkAfter->
        getOriginalGraceNotesGroup ());
  }

  // are there pending chord member notes string numbers?
  if (fPendingChordMemberNotesStringNumbers.size ()) {
    list<int>::const_iterator
      iBegin = fPendingChordMemberNotesStringNumbers.begin (),
      iEnd   = fPendingChordMemberNotesStringNumbers.end (),
      i      = iBegin;

    for ( ; ; ) {
      int stringNumber = (*i);

      fLilypondCodeStream <<
        "\\" <<
        stringNumber;

      if (++i == iEnd) break;
      fLilypondCodeStream <<
        ' ';
    } // for
    fLilypondCodeStream <<
      ' ';

    // forget about the pending string numbers
    fPendingChordMemberNotesStringNumbers.clear ();
  }

  // fetch the chord single tremolo
  S_msrSingleTremolo
    chordSingleTremolo =
      chord->getChordSingleTremolo ();

  if (chordSingleTremolo) {
    // generate code for the chord single tremolo
    fLilypondCodeStream <<
      singleTremoloDurationAsLilypondString (
        chordSingleTremolo);
  }

  fLilypondCodeStream <<
    ' ';

  // get the chord articulations
  list<S_msrArticulation>
    chordArticulations =
      chord->getChordArticulations ();

  // print the chord articulations if any
  if (chordArticulations.size ()) {
    list<S_msrArticulation>::const_iterator i;
    for (
      i=chordArticulations.begin ();
      i!=chordArticulations.end ();
      i++
    ) {
      generateChordArticulation ((*i));

      fLilypondCodeStream <<
        ' ';
    } // for
  }

  // print the chord technicals if any
  list<S_msrTechnical>
    chordTechnicals =
      chord->getChordTechnicals ();

  if (chordTechnicals.size ()) {
    list<S_msrTechnical>::const_iterator i;
    for (
      i=chordTechnicals.begin ();
      i!=chordTechnicals.end ();
      i++
    ) {
      fLilypondCodeStream <<
        technicalAsLilypondString ((*i)) <<
        ' '; // JMI
    } // for
  }

  // print the chord technicals with integer if any
  list<S_msrTechnicalWithInteger>
    chordTechnicalWithIntegers =
      chord->getChordTechnicalWithIntegers ();

  if (chordTechnicalWithIntegers.size ()) {
    list<S_msrTechnicalWithInteger>::const_iterator i;
    for (
      i=chordTechnicalWithIntegers.begin ();
      i!=chordTechnicalWithIntegers.end ();
      i++
    ) {
      fLilypondCodeStream <<
        technicalWithIntegerAsLilypondString ((*i)) <<
        ' '; // JMI
    } // for
  }

  // print the chord technicals with float if any
  list<S_msrTechnicalWithFloat>
    chordTechnicalWithFloats =
      chord->getChordTechnicalWithFloats ();

  if (chordTechnicalWithFloats.size ()) {
    list<S_msrTechnicalWithFloat>::const_iterator i;
    for (
      i=chordTechnicalWithFloats.begin ();
      i!=chordTechnicalWithFloats.end ();
      i++
    ) {
      fLilypondCodeStream <<
        technicalWithFloatAsLilypondString ((*i)) <<
        ' '; // JMI
    } // for
  }

  // print the chord technicals with string if any
  list<S_msrTechnicalWithString>
    chordTechnicalWithStrings =
      chord->getChordTechnicalWithStrings ();

  if (chordTechnicalWithStrings.size ()) {
    list<S_msrTechnicalWithString>::const_iterator i;
    for (
      i=chordTechnicalWithStrings.begin ();
      i!=chordTechnicalWithStrings.end ();
      i++
    ) {
      fLilypondCodeStream <<
        technicalWithStringAsLilypondString ((*i)) <<
        ' '; // JMI
    } // for
  }

  // print the chord ornaments if any
  list<S_msrOrnament>
    chordOrnaments =
      chord->getChordOrnaments ();

  if (chordOrnaments.size ()) {
    list<S_msrOrnament>::const_iterator i;
    for (
      i=chordOrnaments.begin ();
      i!=chordOrnaments.end ();
      i++
    ) {
      S_msrOrnament
        ornament = (*i);

      switch (ornament->getOrnamentPlacementKind ()) {
        case kPlacementNone:
          fLilypondCodeStream << "-";
          break;
        case kPlacementAbove:
          fLilypondCodeStream << "^";
          break;
        case kPlacementBelow:
          fLilypondCodeStream << "-";
          break;
      } // switch

      generateOrnament (ornament); // some ornaments are not yet supported
    } // for
  }

  // print the chord dynamics if any
  list<S_msrDynamics>
    chordDynamics =
      chord->getChordDynamics ();

  if (chordDynamics.size ()) {
    list<S_msrDynamics>::const_iterator i;
    for (
      i=chordDynamics.begin ();
      i!=chordDynamics.end ();
      i++
    ) {
      S_msrDynamics
        dynamics = (*i);

      switch (dynamics->getDynamicsPlacementKind ()) {
        case kPlacementNone:
          fLilypondCodeStream << "-";
          break;
        case kPlacementAbove:
          fLilypondCodeStream << "^";
          break;
        case kPlacementBelow:
          // this is done by LilyPond by default
          break;
      } // switch

      fLilypondCodeStream <<
        dynamicsAsLilypondString (dynamics) << ' ';
    } // for
  }

  // print the chord other dynamics if any
  list<S_msrOtherDynamics>
    chordOtherDynamics =
      chord->getChordOtherDynamics ();

  if (chordOtherDynamics.size ()) {
    list<S_msrOtherDynamics>::const_iterator i;
    for (
      i=chordOtherDynamics.begin ();
      i!=chordOtherDynamics.end ();
      i++
    ) {
      S_msrOtherDynamics
        otherDynamics = (*i);

      switch (otherDynamics->getOtherDynamicsPlacementKind ()) {
        case kPlacementNone:
          fLilypondCodeStream << "-";
          break;
        case kPlacementAbove:
          fLilypondCodeStream << "^";
          break;
        case kPlacementBelow:
          // this is done by LilyPond by default
          break;
      } // switch

      fLilypondCodeStream <<
        "\\otherDynamic \"" <<
        otherDynamics->getOtherDynamicsString () <<
        "\" ";
    } // for
  }

  // print the chord words if any
  list<S_msrWords>
    chordWords =
      chord->getChordWords ();

  if (chordWords.size ()) {
    list<S_msrWords>::const_iterator i;
    for (
      i=chordWords.begin ();
      i!=chordWords.end ();
      i++
    ) {

      msrPlacementKind
        wordsPlacementKind =
          (*i)->getWordsPlacementKind ();

      string wordsContents =
        (*i)->getWordsContents ();

      switch (wordsPlacementKind) {
        case kPlacementNone:
          // should not occur
          break;
        case kPlacementAbove:
          fLilypondCodeStream << "^";
          break;
        case kPlacementBelow:
          fLilypondCodeStream << "_";
          break;
      } // switch

      fLilypondCodeStream <<
        "\\markup" << " { " <<
        doubleQuoteStringIfNonAlpha (wordsContents) <<
        " } ";
    } // for
  }

  // print the chord beams links if any
  list<S_msrChordBeamLink>
    chordBeamLinks =
      chord->getChordBeamLinks ();

  if (chordBeamLinks.size ()) {
    list<S_msrChordBeamLink>::const_iterator i;
    for (
      i=chordBeamLinks.begin ();
      i!=chordBeamLinks.end ();
      i++
    ) {
      S_msrChordBeamLink chordBeamLink = (*i);

      S_msrBeam originalBeam = chordBeamLink->getOriginalBeam ();

      // LilyPond will take care of multiple beams automatically,
      // so we need only generate code for the first number (level)
      switch (originalBeam->getBeamKind ()) {

        case msrBeam::kBeginBeam:
          if (originalBeam->getBeamNumber () == 1)
            fLilypondCodeStream << "[ ";
          break;

        case msrBeam::kContinueBeam:
          break;

        case msrBeam::kEndBeam:
          if (originalBeam->getBeamNumber () == 1)
            fLilypondCodeStream << "] ";
          break;

        case msrBeam::kForwardHookBeam:
          break;

        case msrBeam::kBackwardHookBeam:
          break;

        case msrBeam::k_NoBeam:
          break;
      } // switch
    } // for
  }

  // print the chord slur links if any
  list<S_msrChordSlurLink>
    chordSlurLinks =
      chord->getChordSlurLinks ();

  if (chordSlurLinks.size ()) {
    list<S_msrChordSlurLink>::const_iterator i;
    for (
      i=chordSlurLinks.begin ();
      i!=chordSlurLinks.end ();
      i++
    ) {
      S_msrChordSlurLink chordSlurLink = (*i);

      S_msrSlur originalSlur = chordSlurLink->getOriginalSlur ();

      switch (originalSlur->getSlurTypeKind ()) {
        case msrSlur::k_NoSlur:
          break;
        case msrSlur::kRegularSlurStart:
          fLilypondCodeStream << "( ";
          break;
        case msrSlur::kPhrasingSlurStart:
          fLilypondCodeStream << "\\( ";
          break;
        case msrSlur::kSlurContinue:
          break;
        case msrSlur::kRegularSlurStop:
          fLilypondCodeStream << ") ";
          break;
        case msrSlur::kPhrasingSlurStop:
          fLilypondCodeStream << "\\) ";
          break;
      } // switch
    } // for
  }

/* Don't print the chord ties, rely only on its notes's ties // JMI
  // thus using LilyPond's partially tied chords // JMI
  // print the chord ties if any
  list<S_msrTie>
    chordTies =
      chord->getChordTies ();

  if (chordTies.size ()) {
    list<S_msrTie>::const_iterator i;
    for (
      i=chordTies.begin ();
      i!=chordTies.end ();
      i++
    ) {
      fLilypondCodeStream <<
        "%{line " << inputLineNumber << "%}" <<
        "~ %{S_msrChord}"; // JMI spaces???
    } // for
  }
*/

/* JMI
  // print the tie if any
  {
    S_msrTie chordTie = chord->getChordTie ();

    if (chordTie) {
      if (chordTie->getTieKind () == msrTie::kTieStart) {
        fLilypondCodeStream <<
          "%{line " << inputLineNumber << "%}" <<
          "~ ";
      }
    }
  }
*/

  // get the chord ligatures
  list<S_msrLigature>
    chordLigatures =
      chord->getChordLigatures ();

  // print the chord ligatures if any
  if (chordLigatures.size ()) {
    list<S_msrLigature>::const_iterator i;
    for (
      i=chordLigatures.begin ();
      i!=chordLigatures.end ();
      i++
    ) {
      switch ((*i)->getLigatureKind ()) {
        case msrLigature::kLigatureNone:
          break;
        case msrLigature::kLigatureStart:
          fLilypondCodeStream << "\\[ ";
          break;
        case msrLigature::kLigatureContinue:
          break;
        case msrLigature::kLigatureStop:
          fLilypondCodeStream << "\\] ";
          break;
      } // switch
    } // for
  }

  // print the chord wedges if any
  list<S_msrWedge>
    chordWedges =
      chord->getChordWedges ();

  if (chordWedges.size ()) {
    list<S_msrWedge>::const_iterator i;
    for (
      i=chordWedges.begin ();
      i!=chordWedges.end ();
      i++
      ) {
      switch ((*i)->getWedgeKind ()) {
        case msrWedge::kWedgeKindNone:
          break;
        case msrWedge::kWedgeCrescendo:
          fLilypondCodeStream << "\\< ";
          break;
        case msrWedge::kWedgeDecrescendo:
          fLilypondCodeStream << "\\> ";
          break;
        case msrWedge::kWedgeStop:
          fLilypondCodeStream << "\\! ";
          break;
      } // switch
    } // for
  }

  // get the chord glissandos
  const list<S_msrGlissando>&
    chordGlissandos =
      chord->getChordGlissandos ();

  // print the chord glissandos if any
  if (chordGlissandos.size ()) {
    list<S_msrGlissando>::const_iterator i;
    for (
      i=chordGlissandos.begin ();
      i!=chordGlissandos.end ();
      i++
    ) {
      S_msrGlissando glissando = (*i);

      switch (glissando->getGlissandoTypeKind ()) {
        case msrGlissando::kGlissandoTypeNone:
          break;

        case msrGlissando::kGlissandoTypeStart:
          // generate the glissando itself
          fLilypondCodeStream <<
            "\\glissando ";
          break;

        case msrGlissando::kGlissandoTypeStop:
          break;
      } // switch
    } // for
  }

  // get the chord slides
  const list<S_msrSlide>&
    chordSlides =
      chord->getChordSlides ();

  // print the chord slides if any, implemented as glissandos
  if (chordSlides.size ()) {
    list<S_msrSlide>::const_iterator i;
    for (
      i=chordSlides.begin ();
      i!=chordSlides.end ();
      i++
    ) {
      S_msrSlide slide = (*i);

      switch (slide->getSlideTypeKind ()) {
        case msrSlide::kSlideTypeNone:
          break;

        case msrSlide::kSlideTypeStart:
          // generate the glissando itself
          fLilypondCodeStream <<
            "\\glissando ";
          break;

        case msrSlide::kSlideTypeStop:
          break;
      } // switch
    } // for
  }

  // generate the chord dal segnos if any
  const list<S_msrDalSegno>&
    chordDalSegnos =
      chord->getChordDalSegnos ();

  if (chordDalSegnos.size ()) {
    list<S_msrDalSegno>::const_iterator i;
    for (i=chordDalSegnos.begin (); i!=chordDalSegnos.end (); i++) {
      // generate the dal segno
      fLilypondCodeStream <<
        endl <<
        "\\override Score.RehearsalMark.break-visibility = #begin-of-line-invisible" <<
        endl <<
        "\\tweak self-alignment-X #RIGHT" <<
        endl <<
        "\\mark \\markup { " <<
        (*i)->getDalSegnoString () <<
        " }" <<
        endl;
    } // for
  }
}

void lpsr2lilypondTranslator::generateChordInGraceNotesGroup (S_msrChord chord)
{
  generateCodeRightBeforeChordContents (chord);

  // generate the start of the chord
  fLilypondCodeStream <<
    "<";

  generateCodeForChordInGraceNotesGroupContents (chord);

  // generate the end of the chord
  fLilypondCodeStream <<
    ">";

  generateCodeRightAfterChordContents (chord);
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrChord& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrChord " <<
      elt->asShortString () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

#ifdef TRACING_IS_ENABLED
  if (fOnGoingGraceNotesGroup) {
    int chordInputLineNumber =
      elt->getInputLineNumber ();

    stringstream s;

    s <<
      "% ==> Start visiting grace chords is ignored inside grace notes groups " <<
      elt->asShortString ();

    msrInternalWarning (
      gGlobalOahOahGroup->getInputSourceName (),
      chordInputLineNumber,
      s.str ());

    return;
  }
#endif

  fOnGoingChord = true;
  fCurrentChord = elt;

  generateCodeAHeadOfChordContents (elt);

  generateCodeRightBeforeChordContents (elt);

  // generate the start of the chord
  fLilypondCodeStream <<
    "<";
}

void lpsr2lilypondTranslator::visitEnd (S_msrChord& elt)
{
#ifdef TRACING_IS_ENABLED
  int inputLineNumber =
    elt->getInputLineNumber ();

  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrChord " <<
      elt->asShortString () <<
      ", line " << inputLineNumber <<
      endl;
  }

  if (fOnGoingGraceNotesGroup) {
    msrInternalWarning (
      gGlobalOahOahGroup->getInputSourceName (),
      inputLineNumber,
      "% ==> End visiting grace chords is ignored inside grace notes groups");

    return;
  }
#endif

  // generate the end of the chord
  fLilypondCodeStream <<
    ">";

  generateCodeRightAfterChordContents (elt);

  // if the preceding item is a chord, the first note of the chord
  // is used as the reference point for the octave placement
  // of a following note or chord
  switch (gGlobalLpsr2lilypondOahGroup->getOctaveEntryKind ()) {
    case kOctaveEntryRelative:
      fCurrentOctaveEntryReference =
        elt->getChordNotesVector () [0];
      break;
    case kOctaveEntryAbsolute:
      break;
    case kOctaveEntryFixed:
      break;
  } // switch

  fOnGoingChord = false;
  fCurrentChord = nullptr;
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrTuplet& elt)
{
  int inputLineNumber =
    elt->getInputLineNumber ();

#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrTuplet" <<
      ", line " << inputLineNumber <<
      endl;
  }
#endif

  if (fTupletsStack.size ()) {
    // elt is a nested tuplet

    S_msrTuplet
      containingTuplet =
        fTupletsStack.top ();

    // unapply containing tuplet factor,
    // i.e 3/2 inside 5/4 becomes 15/8 in MusicXML...
    elt->
      unapplySoundingFactorToTupletMembers (
        containingTuplet->
          getTupletFactor ());
  }

  if (gGlobalLpsr2lilypondOahGroup->getIndentTuplets ()) {
    fLilypondCodeStream << endl;
  }

  // get bracket kind
  msrTuplet::msrTupletBracketKind
    tupletBracketKind =
      elt->getTupletBracketKind ();

  switch (tupletBracketKind) {
    case msrTuplet::kTupletBracketYes:
    /* JMI
      fLilypondCodeStream <<
        "%{kTupletBracketYes%}" <<
        endl;
        */
      break;
    case msrTuplet::kTupletBracketNo:
      fLilypondCodeStream <<
        "\\once \\omit TupletBracket" <<
        endl;
      break;
  } // switch

  // get line shape kind
  msrTuplet::msrTupletLineShapeKind
    tupletLineShapeKind =
      elt->getTupletLineShapeKind ();

  switch (tupletLineShapeKind) {
    case msrTuplet::kTupletLineShapeStraight:
      break;
    case msrTuplet::kTupletLineShapeCurved:
      fLilypondCodeStream <<
        "\\temporary \\tupletsCurvedBrackets" <<
        endl;
      break;
  } // switch

  // get show number kind
  msrTuplet::msrTupletShowNumberKind
    tupletShowNumberKind =
      elt->getTupletShowNumberKind ();

  switch (tupletShowNumberKind) {
    case msrTuplet::kTupletShowNumberActual:
    /* JMI
      fLilypondCodeStream <<
        "%{tupletShowNumberActual%}" <<
        endl;
        */
      break;
    case msrTuplet::kTupletShowNumberBoth:
      fLilypondCodeStream <<
        "\\once \\override TupletNumber.text = #tuplet-number::calc-fraction-text" <<
        endl;
      break;
    case msrTuplet::kTupletShowNumberNone:
      fLilypondCodeStream <<
        "\\once \\omit TupletNumber" <<
        endl;
      break;
  } // switch

  // get show type kind
  msrTuplet::msrTupletShowTypeKind
    tupletShowTypeKind =
      elt->getTupletShowTypeKind ();

  rational
    memberNoteDisplayWholeNotes =
      elt->getMemberNotesDisplayWholeNotes ();

  switch (tupletShowTypeKind) {
    case msrTuplet::kTupletShowTypeActual:
      fLilypondCodeStream <<
     // JMI ???   "\\once \\override TupletNumber.text = #(tuplet-number::append-note-wrapper tuplet-number::calc-fraction-text \"" <<
        "\\once \\override TupletNumber.text = #(tuplet-number::append-note-wrapper tuplet-number::calc-denominator-text \"" <<
        wholeNotesAsLilypondString (
          inputLineNumber,
          memberNoteDisplayWholeNotes) <<
        "\")" <<
        endl;
      break;
    case msrTuplet::kTupletShowTypeBoth:
      fLilypondCodeStream <<
        "\\once \\override TupletNumber.text = #(tuplet-number::fraction-with-notes" <<
        " #{ " <<
        wholeNotesAsLilypondString (
          inputLineNumber,
          memberNoteDisplayWholeNotes) <<
        " #} #{ " <<
        wholeNotesAsLilypondString (
          inputLineNumber,
          memberNoteDisplayWholeNotes) <<
        " #})" <<
        endl;
      break;
    case msrTuplet::kTupletShowTypeNone:
    /* JMI
      fLilypondCodeStream <<
        "%{tupletShowTypeNone%}" <<
        endl;
        */
      break;
  } // switch

  fLilypondCodeStream <<
    "\\tuplet " <<
    elt->getTupletFactor ().asRational () <<
    " {" <<
    endl;

  fTupletsStack.push (elt);

  gIndenter++; // decremented in visitEnd (S_msrTuplet& elt)

  // force durations to be displayed explicitly
  // at the beginning of the tuplet
  fLastMetWholeNotes = rational (0, 1);
}

void lpsr2lilypondTranslator::visitEnd (S_msrTuplet& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrTuplet" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  gIndenter--; // incremented in visitStart (S_msrTuplet& elt)

  if (gGlobalLpsr2lilypondOahGroup->getIndentTuplets ()) {
    fLilypondCodeStream << endl;
  }

  fLilypondCodeStream <<
    "}" <<
    endl;

  // get line shape kind
  msrTuplet::msrTupletLineShapeKind
    tupletLineShapeKind =
      elt->getTupletLineShapeKind ();

  switch (tupletLineShapeKind) {
    case msrTuplet::kTupletLineShapeStraight:
      break;
    case msrTuplet::kTupletLineShapeCurved:
      fLilypondCodeStream <<
        "\\undo \\tupletsCurvedBrackets" <<
        endl;
      break;
  } // switch

  fTupletsStack.pop ();

/* JMI
 ?????? fCurrentOctaveEntryReference = nullptr;
  // the first note after the tuplet will become the new reference
  */
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrTie& elt)
{
#ifdef TRACING_IS_ENABLED
  int inputLineNumber =
    elt->getInputLineNumber ();

  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrTie" <<
      ", line " << inputLineNumber <<
      endl;
  }
#endif

  switch (elt->getTieKind ()) {
    case msrTie::kTieNone:
      break;
    case msrTie::kTieStart:
      if (fOnGoingNotesStack.size () > 0) {
        // this precludes generating for the chords' ties,
        // since the last of its notes sets fOnGoingNotesStack.size > 0 to false
        // after code has been generated for it
        fLilypondCodeStream <<
  // JMI        "%{line " << inputLineNumber << "%}" <<
          " ~ ";
      }
      break;
    case msrTie::kTieContinue:
      break;
    case msrTie::kTieStop:
      break;
   } // switch
}

void lpsr2lilypondTranslator::visitEnd (S_msrTie& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrTie" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrSegno& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrSegno" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitStart (S_msrHiddenMeasureAndBarline& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrHiddenMeasureAndBarline" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    "\\HiddenMeasureAndBarLine " <<
    "\\time " <<
    "4/4 ";
}

void lpsr2lilypondTranslator::visitStart (S_msrCoda& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrCoda" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrEyeGlasses& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting eyeGlasses" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    "^\\markup {\\eyeglasses} ";
}

void lpsr2lilypondTranslator::visitStart (S_msrScordatura& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting scordatura" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

/* JMI
  const list<S_msrStringTuning>&
    scordaturaStringTuningsList =
      elt->getScordaturaStringTuningsList ();

  gIndenter++;

  fLilypondCodeStream <<
    "\\new Staff {" <<
    endl;

  gIndenter++;

  fLilypondCodeStream <<
    "\\hide Staff.Stem" <<
    endl <<
    "\\hide Staff.TimeSignature" <<
    endl <<
    "\\small" <<
    endl <<
    "<";

  if (scordaturaStringTuningsList.size ()) {
    list<S_msrStringTuning>::const_iterator
      iBegin = scordaturaStringTuningsList.begin (),
      iEnd   = scordaturaStringTuningsList.end (),
      i      = iBegin;
    for ( ; ; ) {
      S_msrStringTuning
        stringTuning = (*i);

      fLilypondCodeStream <<
        stringTuningAsLilypondString (
          elt->getInputLineNumber (),
          stringTuning);

      if (++i == iEnd) break;

      fLilypondCodeStream << ' ';
    } // for
  }

  fLilypondCodeStream <<
    ">4" <<
    endl <<
    "}" <<
    endl;

  gIndenter--;

  fLilypondCodeStream <<
    "{ c'4 }" <<
    endl <<

  gIndenter--;
  */
}

void lpsr2lilypondTranslator::visitStart (S_msrPedal& elt)
{
  int inputLineNumber =
    elt->getInputLineNumber ();

#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting pedal" <<
      ", line " << inputLineNumber <<
      endl;
  }
#endif

  fLilypondCodeStream << endl;

  switch (elt->getPedalTypeKind ()) {
    case msrPedal::kPedalStart:
      fLilypondCodeStream <<
        "\\sustainOn";
      break;
    case msrPedal::kPedalContinue:
      fLilypondCodeStream <<
        "\\sustainOff\\sustainOn"; // JMI
      break;
    case msrPedal::kPedalChange:
      fLilypondCodeStream <<
        "\\sustainOff\\sustainOn";
      break;
    case msrPedal::kPedalStop:
      fLilypondCodeStream <<
        "\\sustainOff";
      break;
    case msrPedal::k_NoPedalType:
      {
        // should not occur

        stringstream s;

        s <<
          "msrPedal '" <<
          elt->asShortString () <<
          "' has no pedal type";

        msrInternalError (
          gGlobalOahOahGroup->getInputSourceName (),
          inputLineNumber,
          __FILE__, __LINE__,
          s.str ());
      }
      break;
  } // switch

  fLilypondCodeStream << endl;
}

void lpsr2lilypondTranslator::visitStart (S_msrDamp& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting damp" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    "^\\markup {\\damp} ";
}

void lpsr2lilypondTranslator::visitStart (S_msrDampAll& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting dampAll" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    "^\\markup {\\dampAll} ";
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrBarline& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      endl <<
      "% --> Start visiting msrBarline" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  int inputLineNumber =
    elt->getInputLineNumber ();

  switch (elt->getBarlineCategory ()) {
    case msrBarline::kBarlineCategoryStandalone:
      switch (elt->getBarlineStyleKind ()) {
        case msrBarline::kBarlineStyleNone:
          break;
        case msrBarline::kBarlineStyleRegular:
          fLilypondCodeStream << "\\bar \"|\" ";
          break;
        case msrBarline::kBarlineStyleDotted:
          fLilypondCodeStream << "\\bar \";\" ";
          break;
        case msrBarline::kBarlineStyleDashed:
          fLilypondCodeStream << "\\bar \"!\" ";
          break;
        case msrBarline::kBarlineStyleHeavy:
          fLilypondCodeStream << "\\bar \".\" ";
          break;
        case msrBarline::kBarlineStyleLightLight:
          fLilypondCodeStream << "\\bar \"||\" ";
          break;
        case msrBarline::kBarlineStyleLightHeavy:
          fLilypondCodeStream <<
            endl <<
            "\\bar \"|.\" ";
          break;
        case msrBarline::kBarlineStyleHeavyLight:
          fLilypondCodeStream << "\\bar \".|\" ";
          break;
        case msrBarline::kBarlineStyleHeavyHeavy:
          fLilypondCodeStream << "\\bar \"..\" ";
          break;
        case msrBarline::kBarlineStyleTick:
          fLilypondCodeStream << "\\bar \"'\" ";
          break;
        case msrBarline::kBarlineStyleShort:
          // \bar "/" is the custom short barline
          fLilypondCodeStream << "\\bar \"/\" ";
          break;
          /* JMI
        case msrBarline::kBarlineStyleNone:
          fLilypondCodeStream << "\\bar \"\" ";
          break;
          */
      } // switch

      if (gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()) {
        // print the barline line number as a comment
        fLilypondCodeStream <<
          "%{ " << inputLineNumber << " %} ";
      }

      fLilypondCodeStream << endl;

/* JMI BOF ???
      switch (elt->getBarlineHasSegnoKind ()) {
        case msrBarline::kBarlineHasSegnoYes:
          // generate the segno
          generateSegno ((*i));
          break;
        case msrBarline::kBarlineHasSegnoNo:
          break;
      } // switch

      switch (elt->getBarlineHasCodaKind ()) {
        case msrBarline::kBarlineHasCodaYes:
          fLilypondCodeStream <<
            "\\mark \\markup { \\musicglyph #\"scripts.coda\" } ";
          if (gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()) {
            // print the input line number as a comment
            fLilypondCodeStream <<
              "%{line " << note->getInputLineNumber () << "%} ";
          }
          fLilypondCodeStream << endl;
          break;
        case msrBarline::kBarlineHasCodaNo:
          break;
      } // switch
*/
      break;

    case msrBarline::kBarlineCategoryRepeatStart:
      if (gGlobalLpsr2lilypondOahGroup->getKeepRepeatBarlines ()) {
      /*
        if (gGlobalLpsr2lilypondOahGroup->getRepeatBrackets ()) {
          if (fRepeatDescrsStack.size ()) {
            S_msrRepeat
              currentRepeat =
                fRepeatDescrsStack.back ()->getRepeat (),
              precedingRepeat =
                currentRepeat->getImmediatelyPrecedingRepeat ();

            if (precedingRepeat && ! precedingRepeat-> getRepeatEndings ().size ()) {
              // JMI
            }
          }
          else {
            // JMI
          }
          fLilypondCodeStream << "\\bar \"[|:\" ";
        }
      */
        fLilypondCodeStream << "\\bar \".|:\" ";
      }
      else {
        // JMI
      }
      break;

    case msrBarline::kBarlineCategoryRepeatEnd:
      if (gGlobalLpsr2lilypondOahGroup->getKeepRepeatBarlines ()) {
        if (gGlobalLpsr2lilypondOahGroup->getRepeatBrackets ()) {
          if (fRepeatDescrsStack.size ()) {
            S_msrRepeat
              currentRepeat =
                fRepeatDescrsStack.back ()->getRepeat (),
              followingRepeat =
                currentRepeat->getImmediatelyPrecedingRepeat ();

      // JMI      if (followingRepeat && ! followingRepeat-> getRepeatEndings ().size ()) {
            if (followingRepeat) {
              fLilypondCodeStream << "\\bar \":|][|:\" ";
            }
          }
          else {
            fLilypondCodeStream << "\\bar \":|.\" ";
          }
        }
        else {
          fLilypondCodeStream << "\\bar \":|.\" ";
        }
      }
      else {
        // JMI
      }
      break;

    case msrBarline::kBarlineCategoryHookedEndingStart:
    case msrBarline::kBarlineCategoryHookedEndingEnd:
    case msrBarline::kBarlineCategoryHooklessEndingStart:
    case msrBarline::kBarlineCategoryHooklessEndingEnd:
      // should not occur, since
      // LilyPond will take care of displaying the repeat
      break;

    case msrBarline::k_NoBarlineCategory:
      {
        stringstream s;

        s <<
          "barline category has not been set" <<
          ", line " << elt->getInputLineNumber ();

  // JMI      msrInternalError (
        msrInternalWarning (
          gGlobalOahOahGroup->getInputSourceName (),
          inputLineNumber,
  // JMI        __FILE__, __LINE__,
          s.str ());
      }
      break;
  } // switch

  if (gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()) {
    fLilypondCodeStream <<
      " %{ " << inputLineNumber << " %}";
  }
}

void lpsr2lilypondTranslator::visitEnd (S_msrBarline& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      endl <<
      "% --> End visiting msrBarline" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrBarCheck& elt)
{
#ifdef TRACING_IS_ENABLED
  int inputLineNumber =
    elt->getInputLineNumber ();

  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrBarCheck" <<
      ", nextBarNumber: " <<
      elt->getNextBarPuristNumber () <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  int nextBarPuristNumber =
    elt->getNextBarPuristNumber ();

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceRestMeasures ()) {
    fLilypondCodeStream <<
      "% nextBarPuristNumber = " <<
      nextBarPuristNumber <<
      ", fOnGoingRestMeasures = " <<
      "fOnGoingVoiceCadenza = " <<
      booleanAsString (fOnGoingVoiceCadenza) <<
      ", nextBarPuristNumber = " <<
      booleanAsString (fOnGoingRestMeasures) <<
      ", line " << inputLineNumber <<
      endl;
  }
#endif

  if (
    ! fOnGoingVoiceCadenza
      // should be tested in msr2lpsrTranslator.cpp JMI visitEnd (S_msrMeasure&)
      // MusicXML bar numbers cannot be relied upon for a LilyPond bar number check
    &&
    ! fOnGoingRestMeasures
  ) {
    // don't generate a bar check before the end of measure 1 // JMI ???
    fLilypondCodeStream <<
      "| % " << nextBarPuristNumber;

    if (gGlobalLpsr2lilypondOahGroup->getOriginalMeasureNumbers ()) {
      // print the original MusicXML measure number as a comment
      fLilypondCodeStream <<
        " (omn: " << elt->getNextBarOriginalNumber () << ")";
    }

    fLilypondCodeStream <<
      endl;
  }
}

void lpsr2lilypondTranslator::visitEnd (S_msrBarCheck& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrBarCheck" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrBarNumberCheck& elt)
{
#ifdef TRACING_IS_ENABLED
  int inputLineNumber =
    elt->getInputLineNumber ();

  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrBarNumberCheck" <<
      ", line " << inputLineNumber <<
      endl;
  }
#endif

  if (gGlobalLpsr2lilypondOahGroup->getNoBarNumberChecks ()) {
    return;
  }

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceRestMeasures ()) {
    fLilypondCodeStream <<
      "% fOnGoingVoiceCadenza = " <<
      booleanAsString (fOnGoingVoiceCadenza) <<
      ", fOnGoingRestMeasures = " <<
      booleanAsString (fOnGoingRestMeasures) <<
      ", line " << inputLineNumber <<
      endl;
  }
#endif

  if (
    ! fOnGoingVoiceCadenza
      // should be tested in msr2lpsrTranslator.cpp JMI visitEnd (S_msrMeasure&)
     // MusicXML bar numbers cannot be relied upon for a LilyPond bar number check
    &&
    ! fOnGoingRestMeasures
  ) {
    int nextBarPuristNumber =
      elt->getNextBarPuristNumber ();

    fLilypondCodeStream <<
      "\\barNumberCheck #" <<
      nextBarPuristNumber;

    if (gGlobalLpsr2lilypondOahGroup->getOriginalMeasureNumbers ()) {
      fLilypondCodeStream <<
        " %{omn: " <<
        elt->getNextBarOriginalNumber () <<
        "%}";
    }

    fLilypondCodeStream <<
      endl;
  }
}

void lpsr2lilypondTranslator::visitEnd (S_msrBarNumberCheck& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrBarNumberCheck" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrLineBreak& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrLineBreak" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    "\\myBreak | % " << elt->getNextBarNumber ();

    if (gGlobalLpsr2lilypondOahGroup->getOriginalMeasureNumbers ()) {
      // print the original MusicXML measure number as a comment
      fLilypondCodeStream <<
        " (omn: " << elt->getNextBarNumber () << ")";
    }

  fLilypondCodeStream <<
    endl << endl;
}

void lpsr2lilypondTranslator::visitEnd (S_msrLineBreak& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrLineBreak" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrPageBreak& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrPageBreak" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    "\\myPageBreak " <<
    endl << endl;
}

void lpsr2lilypondTranslator::visitEnd (S_msrPageBreak& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrPageBreak" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrRepeat& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrRepeat" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  int repeatEndingsNumber =
    elt->getRepeatEndings ().size ();

  if (repeatEndingsNumber == 0)
    repeatEndingsNumber = 2; // implicitly JMI ???

  fRepeatDescrsStack.push_back (
    lpsrRepeatDescr::create (
      elt,
      repeatEndingsNumber));

  int
    repeatTimes =
      elt->getRepeatTimes ();

  stringstream s;

  fLilypondCodeStream << endl;

  s <<
    "\\repeat volta " <<
    repeatTimes <<
// JMI    fRepeatDescrsStack.back ()->getRepeatEndingsNumber () <<
    " {";

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      s.str () << "% start of repeat";
  }
  else {
    fLilypondCodeStream <<
      s.str ();
  }

  fLilypondCodeStream << endl;

  gIndenter++; // decremented in visitEnd (S_msrRepeat& elt)

  if (repeatTimes > 2) {
    fLilypondCodeStream <<
      "<>^\"" << repeatTimes << " times\"" << // JMI
      endl;
  }
}

void lpsr2lilypondTranslator::visitEnd (S_msrRepeat& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrRepeat" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  /*
    CAUTION:
      the end of the repeat has to be generated
      before the endings are handled
  */

  if (! fRepeatDescrsStack.back ()->getEndOfRepeatHasBeenGenerated ()) {
    // the end of the repeat has not been generated yet JMI

    gIndenter--;

    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
      fLilypondCodeStream << left <<
        setw (commentFieldWidth) <<
        "}" << "% end of repeat" <<
        endl;
    }
    else {
      fLilypondCodeStream <<
        endl <<
        "}" <<
        endl << endl;
    }
  }

  //   gIndenter--; // incremented in visitStart (S_msrRepeat& elt) JMI ???

  fRepeatDescrsStack.pop_back ();
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrRepeatCommonPart& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrRepeatCommonPart" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrRepeatCommonPart& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrRepeatCommonPart" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrRepeatEnding& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrRepeatEnding" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fRepeatDescrsStack.back ()->
    incrementRepeatEndingsCounter ();

  int
    repeatEndingInternalNumber =
      elt->getRepeatEndingInternalNumber ();

  if (repeatEndingInternalNumber == 1) {
    gIndenter--;

    // first repeat ending is in charge of
    // outputting the end of the repeat
    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
      fLilypondCodeStream <<
        setw (commentFieldWidth) << left <<
        "}" << "% end of repeat" <<
        endl;
    }
    else {
      fLilypondCodeStream <<
        endl <<
        "}" <<
        endl;
    }
    fRepeatDescrsStack.back ()->
      setEndOfRepeatHasBeenGenerated ();

    // first repeat ending is in charge of
    // outputting the start of the alternative
    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
      fLilypondCodeStream << left <<
        endl <<
        setw (commentFieldWidth) <<
        "\\alternative {" <<
        "% start of alternative" <<
        endl;
    }
    else {
      fLilypondCodeStream <<
        endl <<
        "\\alternative {" <<
        endl;
    }

    gIndenter++;
  }

  // output the start of the ending
  switch (elt->getRepeatEndingKind ()) {
    case msrRepeatEnding::kHookedEnding:
      if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
        fLilypondCodeStream << left <<
          setw (commentFieldWidth) <<
          "{" << "% start of repeat hooked ending" <<
          endl;
      }
      else {
        fLilypondCodeStream <<
          "{" <<
          endl;
      }
      break;

    case msrRepeatEnding::kHooklessEnding:
      if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
        fLilypondCodeStream << left <<
          setw (commentFieldWidth) <<
          "{" << "% start of repeat hookless ending" <<
          endl;
      }
      else {
        fLilypondCodeStream <<
          "{" <<
          endl;
      }
      break;
  } // switch

  gIndenter++;

  // generate the repeat ending number if any
  string
    repeatEndingNumber =
      elt->getRepeatEndingNumber ();

  if (repeatEndingNumber.size ()) {
  /* only if numbers differ JMI ???
    if (repeatEndingInternalNumber == 1) {
      fLilypondCodeStream <<
        "\\set Score.repeatCommands = #'((volta \"" <<
        repeatEndingNumber <<
        "\"))" <<
        endl;
    }
    else {
      fLilypondCodeStream <<
        "\\set Score.repeatCommands = #'(end-repeat (volta \"" <<
        repeatEndingNumber <<
        "\"))" <<
        endl;
    }
    */
  }
}

void lpsr2lilypondTranslator::visitEnd (S_msrRepeatEnding& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrRepeatEnding" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  gIndenter--;

  // output the end of the ending
  fLilypondCodeStream << endl;

  switch (elt->getRepeatEndingKind ()) {
    case msrRepeatEnding::kHookedEnding:
      if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
        fLilypondCodeStream << left <<
          setw (commentFieldWidth) <<
          "}" << "% end of repeat hooked ending" <<
          endl;
      }
      else {
        fLilypondCodeStream <<
          "}" <<
          endl;
      }
      break;

    case msrRepeatEnding::kHooklessEnding:
      if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ())   {
        fLilypondCodeStream << left <<
          setw (commentFieldWidth) <<
          "}" << "% end of repeat hookless ending" <<
          endl;
      }
      else {
        fLilypondCodeStream <<
          "}" <<
          endl;
      }
      break;
  } // switch

/* JMI
#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceRepeats ()) {
    fLilypondCodeStream <<
      "% ===**** fRepeatDescrsStack.back () = '" <<
      fRepeatDescrsStack.back ()->repeatDescrAsString () <<
      "'" <<
      endl;
  }
#endif
*/

  if (
    fRepeatDescrsStack.back ()->getRepeatEndingsCounter ()
      ==
    fRepeatDescrsStack.back ()->getRepeatEndingsNumber ()) {

    gIndenter--; // JMI ???

    // last repeat ending is in charge of
    // outputting the end of the alternative
    if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ())   {
      fLilypondCodeStream << left <<
        setw (commentFieldWidth) <<
        "}" << "% end of alternative" <<
        endl;
    }
    else {
      fLilypondCodeStream <<
        "}" <<
        endl << endl;
    }
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrComment& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrComment" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    "% " << elt->getContents () <<
    endl;

  if (elt->getCommentGapKind () == lpsrComment::kGapAfterwards)
    fLilypondCodeStream << endl;
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrComment& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrComment" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_lpsrSchemeFunction& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting lpsrSchemeFunction" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream <<
    endl <<
    "% Scheme function(s): \"" << elt->getFunctionName () << "\"" <<
    // endl is in the decription
    elt->getFunctionDescription () <<
    // endl is in the decription
    elt->getFunctionCode () <<
    endl << endl;
}

void lpsr2lilypondTranslator::visitEnd (S_lpsrSchemeFunction& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting lpsrSchemeFunction" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrRehearsal& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrRehearsal" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  fLilypondCodeStream << endl;

  switch (elt->getRehearsalPlacementKind ()) {
    case msrPlacementKind::kPlacementNone:
      break;
    case msrPlacementKind::kPlacementAbove:
      break;
    case msrPlacementKind::kPlacementBelow:
      fLilypondCodeStream <<
        "\\once\\override Score.RehearsalMark.direction = #DOWN";
      break;
    } // switch

  fLilypondCodeStream <<
    endl <<
    "\\mark\\markup { ";

  switch (elt->getRehearsalKind ()) {
    case msrRehearsal::kNone:
      fLilypondCodeStream <<
        "\\box"; // default value
      break;
    case msrRehearsal::kRectangle:
      fLilypondCodeStream <<
        "\\box";
      break;
    case msrRehearsal::kOval:
      fLilypondCodeStream <<
        "\\oval";
      break;
    case msrRehearsal::kCircle:
      fLilypondCodeStream <<
        "\\circle";
      break;
    case msrRehearsal::kBracket:
      fLilypondCodeStream <<
        "\\bracket";
      break;
    case msrRehearsal::kTriangle:
      fLilypondCodeStream <<
        "%{\\triangle???%}";
      break;
    case msrRehearsal::kDiamond:
      fLilypondCodeStream <<
        "%{\\diamond???";
      break;
  } // switch

  fLilypondCodeStream <<
    " { \"" <<
    elt->getRehearsalText () <<
    "\"" "}}" <<
    endl;
}

void lpsr2lilypondTranslator::visitEnd (S_msrRehearsal& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrRehearsal" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrMeasuresRepeat& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrMeasuresRepeat" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

#ifdef TRACING_IS_ENABLED
  int repeatReplicasMeasuresNumber =
    elt->fetchMeasuresRepeatReplicasMeasuresNumber ();
#endif

  int replicasNumber =
    elt->fetchMeasuresRepeatReplicasNumber ();

#ifdef TRACING_IS_ENABLED
  if (gGlobalTraceOahGroup->getTraceMeasuresRepeats ()) {
    int repeatPatternMeasuresNumber =
      elt->fetchMeasuresRepeatPatternMeasuresNumber ();

    fLilypondCodeStream <<
      "% measures repeat, line " << elt->getInputLineNumber () << ":" <<
      endl;

    const int fieldWidth = 24;

    fLilypondCodeStream << left <<
      setw (fieldWidth) <<
      "% repeatPatternMeasuresNumber" << " = " << repeatPatternMeasuresNumber <<
      endl <<
      setw (fieldWidth) <<
      "% repeatReplicasMeasuresNumber" << " = " << repeatReplicasMeasuresNumber <<
      endl <<
      setw (fieldWidth) <<
      "% replicasNumber" << " = " << replicasNumber <<
      endl;
  }
#endif

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      "% start of measures repeat" <<
      singularOrPlural (
        elt->fetchMeasuresRepeatReplicasNumber (),
        "replica",
        "replicas") <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }

  fLilypondCodeStream <<
    endl << endl <<
    "\\repeat percent " <<
    replicasNumber + 1 <<
     " { " <<
    endl;

  gIndenter++; // decremented in visitEnd (S_msrMeasuresRepeat&)
}

void lpsr2lilypondTranslator::visitEnd (S_msrMeasuresRepeat& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrMeasuresRepeat" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  gIndenter--; // incremented in visitStart (S_msrMeasuresRepeat&)

  fLilypondCodeStream <<
    endl << endl <<
    " }" <<
    endl;

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream <<
      setw (commentFieldWidth) << left <<
      "% end of measures repeat" <<
      singularOrPlural (
        elt->fetchMeasuresRepeatReplicasNumber (),
        "replica",
        "replicas") <<
      ", line " << elt->getInputLineNumber () <<
      endl << endl;
  }
}

void lpsr2lilypondTranslator::visitStart (S_msrMeasuresRepeatPattern& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "%--> Start visiting msrMeasuresRepeatPattern" <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitEnd (S_msrMeasuresRepeatPattern& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "%--> End visiting msrMeasuresRepeatPattern" <<
      endl;
  }
#endif
}

void lpsr2lilypondTranslator::visitStart (S_msrMeasuresRepeatReplicas& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "%--> Start visiting msrMeasuresRepeatReplicas" <<
      endl;
  }
#endif

  // output the start of the ending
  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      "{" << "% start of measures repeat replicas" <<
      endl;
  }
  else {
    fLilypondCodeStream <<
      endl <<
      "{" <<
      endl;
  }

  gIndenter++; // decremented in visitEnd (S_msrMeasuresRepeat&)
}

void lpsr2lilypondTranslator::visitEnd (S_msrMeasuresRepeatReplicas& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "%--> End visiting msrMeasuresRepeatReplicas" <<
      endl;
  }
#endif

  gIndenter--; // incremented in visitStart (S_msrMeasuresRepeatReplicas&)
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrRestMeasures& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrRestMeasures" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  int inputLineNumber =
    elt->getInputLineNumber ();

  int restMeasuresNumber =
    elt->getRestMeasuresNumber ();

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      "% start of rest measures" <<
      singularOrPlural (
        restMeasuresNumber,
        "measure",
        "measures") <<
      ", line " << inputLineNumber <<
      endl << endl;

    gIndenter++; // decremented in visitEnd (S_msrRestMeasures&)
  }

  fOnGoingRestMeasures = true;
}

void lpsr2lilypondTranslator::visitEnd (S_msrRestMeasures& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrRestMeasures" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  int inputLineNumber =
    elt->getInputLineNumber ();

  int restMeasuresNumber =
    elt->getRestMeasuresNumber ();

  // start counting measures
  fRemainingRestMeasuresNumber =
    elt->getRestMeasuresNumber ();

  // get rest measures sounding notes
  rational
    restMeasuresMeasureSoundingNotes =
      elt->getRestMeasuresMeasureSoundingNotes ();

  // generate rest measures compression if relevant
  // right befoe Ri*n, because if affects only the next music element
  if (
    fCurrentVoice->getVoiceContainsRestMeasures ()
      ||
    gGlobalLpsr2lilypondOahGroup->getCompressFullMeasureRests ()
  ) {
    fLilypondCodeStream <<
      "\\compressFullBarRests" << // JMI
      endl;
  }

  // generate rest measures only now, in case there are
  // clef, keys or times before them in the first measure
  fLilypondCodeStream <<
    "R%{8%}" <<
    restMeasuresWholeNoteAsLilypondString (
      inputLineNumber,
      restMeasuresMeasureSoundingNotes);

  if (restMeasuresNumber > 1) {
    fLilypondCodeStream <<
      "*" <<
      restMeasuresNumber;
  }

  if (gGlobalLpsr2lilypondOahGroup->getInputLineNumbers ()) {
    // print the rest measures line number as a comment
    fLilypondCodeStream <<
      " %{ " << inputLineNumber << " %} ";
  }

  // wait until all measures have be visited
  // before the bar check is generated // JMI ???

  // now we can generate the bar check
  fLilypondCodeStream <<
    " | % " <<
    elt->getRestMeasuresLastMeasurePuristMeasureNumber () + 1;

/* TO BE FINALIZED JMI
    if (gGlobalLpsr2lilypondOahGroup->getOriginalMeasureNumbers ()) {
      // print the original MusicXML measure number as a comment
      fLilypondCodeStream <<
        " (mxml3: " << measureElement->getInputLineNumber () << ")";
    }
*/

  fLilypondCodeStream <<
    endl;

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    gIndenter--; // incremented in visitStart (S_msrRestMeasures&)

    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      "% end of rest measures" <<
      singularOrPlural (
        restMeasuresNumber,
        "measure",
        "measures") <<
      ", line " << inputLineNumber <<
      endl << endl;
  }

  fOnGoingRestMeasures = false;
}

void lpsr2lilypondTranslator::visitStart (S_msrRestMeasuresContents& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "%--> Start visiting msrRestMeasuresContents" <<
      endl;
  }
#endif

  int inputLineNumber =
    elt->getInputLineNumber ();

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      "% start of rest measures contents " <<
      /* JMI
      singularOrPlural (
        restMeasuresNumber,
        "measure",
        "measures") <<
        */
      ", line " << inputLineNumber <<
      endl << endl;

    gIndenter++; // decremented in visitEnd (S_msrRestMeasures&)
  }
}

void lpsr2lilypondTranslator::visitEnd (S_msrRestMeasuresContents& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "%--> End visiting msrRestMeasuresContents" <<
      endl;
  }
#endif

  int inputLineNumber =
    elt->getInputLineNumber ();

  if (gGlobalLpsr2lilypondOahGroup->getLilyPondComments ()) {
    fLilypondCodeStream << left <<
      setw (commentFieldWidth) <<
      "% end of rest measures contents " <<
      /* JMI
      singularOrPlural (
        restMeasuresNumber,
        "measure",
        "measures") <<
        */
      ", line " << inputLineNumber <<
      endl << endl;

    gIndenter--; // incremented in visitStart (S_msrRestMeasures&)
  }
}

//________________________________________________________________________
void lpsr2lilypondTranslator::visitStart (S_msrMidiTempo& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> Start visiting msrMidiTempo" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif

  if (gGlobalLpsr2lilypondOahGroup->getNoMidi ()) {
    fLilypondCodeStream <<
      "%{" <<
      endl;

     gIndenter++;
  }

  fLilypondCodeStream <<
    "\\midi" << " {" <<
    endl;

  gIndenter++;

  if (gGlobalLpsr2lilypondOahGroup->getNoMidi ()) {
    fLilypondCodeStream <<
      "% ";
  }

  fLilypondCodeStream <<
    "\\tempo " <<
    elt->getMidiTempoDuration () << // BLARK
    " = " <<
    elt->getMidiTempoPerSecond () <<
    endl;

  gIndenter--;

  fLilypondCodeStream <<
    "}" <<
    endl;

  if (gGlobalLpsr2lilypondOahGroup->getNoMidi ()) {
    gIndenter--;

    fLilypondCodeStream <<
      "%}" <<
      endl;
  }
}

void lpsr2lilypondTranslator::visitEnd (S_msrMidiTempo& elt)
{
#ifdef TRACING_IS_ENABLED
  if (gGlobalLpsrOahGroup->getTraceLpsrVisitors ()) {
    fLilypondCodeStream <<
      "% --> End visiting msrMidiTempo" <<
      ", line " << elt->getInputLineNumber () <<
      endl;
  }
#endif
}


} // namespace
