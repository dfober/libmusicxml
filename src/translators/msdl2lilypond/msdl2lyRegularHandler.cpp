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

#include "enableExtraOahIfDesired.h"

#include "enableTracingIfDesired.h"
#ifdef TRACING_IS_ENABLED
  #include "traceOah.h"
#endif

#include "lpsr.h"

#include "oahOah.h"

#include "generalOah.h"

#include "musicxmlOah.h"
#include "mxmlTreeOah.h"
#include "mxmlTree2msrOah.h"
#include "msrOah.h"
#include "msr2lpsrOah.h"
#include "lpsrOah.h"
#include "lpsr2lilypondOah.h"
#include "lilypondOah.h"

#include "version.h"

#include "msdl2lyRegularHandler.h"


using namespace std;

namespace MusicXML2
{

//______________________________________________________________________________
S_msdl2lyRegularHandler msdl2lyRegularHandler::create (
  string      executableName,
  string      handlerHeader,
  S_msdl2lyInsiderHandler
              insiderOahHandler)
{
  // create the regular handler
  msdl2lyRegularHandler* o = new
    msdl2lyRegularHandler (
      executableName,
      handlerHeader,
      insiderOahHandler);
  assert (o!=0);

  return o;
}

msdl2lyRegularHandler::msdl2lyRegularHandler (
  string      executableName,
  string      handlerHeader,
  S_msdl2lyInsiderHandler
              insiderOahHandler)
  : oahRegularHandler (
      executableName,
      handlerHeader,
      insiderOahHandler)
{
  // this is done only only after the constructor has been executed,
  // because it uses pure virtual methods
  initializeOahRegularHandler ();

  // create the regular handler groups
  createRegularHandlerGroups ();

#ifdef TRACING_IS_ENABLED
  if (getTraceOah ()) {
    // print the options handler initial state
    gLogStream <<
      "msdl2lyRegularHandler \"" <<
      fHandlerHeader <<
      "\" has been initialized as:" <<
      endl;
  }
#endif
  }

msdl2lyRegularHandler::~msdl2lyRegularHandler ()
{}

void msdl2lyRegularHandler::createRegularHandlerGroups ()
{
#ifdef TRACING_IS_ENABLED
  if (getTraceOah ()) {
    gLogStream <<
      "Creating the regular handler groups for \"" << fHandlerHeader << "\"" <<
      endl;
  }
#endif

  createInformationsRegularGroup ();

  createFilesRegularGroup ();

  createOahRegularGroup ();

  createWarningAndErrorsRegularGroup ();

  createInputRegularGroup ();

  createPartsRegularGroup ();
  createStavesRegularGroup ();
  createVoicesRegularGroup ();

  createTranspositionsRegularGroup ();

  createClefsRegularGroup ();
  createKeysRegularGroup ();
  createTimesRegularGroup ();

  createRepeatsRegularGroup ();

  createMeasuresRegularGroup ();

  createRestsRegularGroup ();
  createNotesRegularGroup ();

  creatBeamsRegularGroup ();

  createArticulationsRegularGroup ();

  createOrnamentsRegularGroup ();

  createGraceNotesRegularGroup ();

  createChordsRegularGroup ();

  createTiesRegularGroup ();
  createSlursRegularGroup ();
  createLigaturesRegularGroup ();

  createDynamicsRegularGroup ();
  createWedgesRegularGroup ();

  createTupletsRegularGroup ();

  createLyricsRegularGroup ();

  createStringsRegularGroup ();

  createTablaturesRegularGroup ();

  createHarmoniesRegularGroup ();

  createFiguredBassesRegularGroup ();

  createBookRegularGroup ();
  createHeaderRegularGroup ();
  createPaperRegularGroup ();
  createLayoutRegularGroup ();

  createOutputRegularGroup ();

  createMidiRegularGroup ();

#ifdef TRACING_IS_ENABLED
  if (getTraceOah ()) {
    gLogStream <<
      "All the regular handler groups for \"" <<
      fHandlerHeader <<
      "\" have been created" <<
      endl;
  }
#endif
}

void msdl2lyRegularHandler::createOahRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Options and help group",
        "hoah-group", "help-oah-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Options and help",
        "hoah", "help-oah",
        "",
        oahElementVisibilityKind::kElementVisibilityHeaderOnly,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms from the insider handler

  registerAtomInRegularSubgroup (K_INSIDER_OPTION_NAME, subGroup);
  registerAtomInRegularSubgroup (K_REGULAR_OPTION_NAME, subGroup);

  registerAtomInRegularSubgroup ("help", subGroup);
  registerAtomInRegularSubgroup ("help-summary", subGroup);

  registerAtomInRegularSubgroup ("help-options-usage", subGroup);

  registerAtomInRegularSubgroup ("name-help", subGroup);
  registerAtomInRegularSubgroup ("apropos-option-name", subGroup);
  registerAtomInRegularSubgroup ("find-string", subGroup);

  registerAtomInRegularSubgroup ("display-options-values", subGroup);

  registerAtomInRegularSubgroup ("show-options-and-arguments", subGroup);

  registerAtomInRegularSubgroup ("display-partgroups", subGroup);

  registerAtomInRegularSubgroup ("display-msr-skeleton", subGroup);
  registerAtomInRegularSubgroup ("display-msr", subGroup);
  registerAtomInRegularSubgroup ("display-lpsr", subGroup);

#ifdef TRACING_IS_ENABLED
  registerAtomInRegularSubgroup (K_TRACE_OAH_LONG_OPTION_NAME, subGroup);
  registerAtomInRegularSubgroup ("trace-oah-details", subGroup);
  registerAtomInRegularSubgroup ("trace-passes", subGroup);

  registerAtomInRegularSubgroup ("trace-msr", subGroup);
  registerAtomInRegularSubgroup ("trace-msr-durations", subGroup);
#endif
}

void msdl2lyRegularHandler::createInformationsRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Informations group",
        "hinfos-group", "help-informations-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Informations",
        "hinfos", "help-informations",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms from the insider handler

  registerAtomInRegularSubgroup ("version", subGroup);
  registerAtomInRegularSubgroup ("about", subGroup);
  registerAtomInRegularSubgroup ("contact", subGroup);

  registerAtomInRegularSubgroup ("display-cpu-usage", subGroup);
}

void msdl2lyRegularHandler::createWarningAndErrorsRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Warning and errors group",
        "hwae-group", "help-warning-and-errors-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Warning and errors",
        "hwae", "help-warning-and-errors",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("quiet", subGroup);
  registerAtomInRegularSubgroup ("dont-show-errors", subGroup);
  registerAtomInRegularSubgroup ("dont-quit-on-errors", subGroup);
}

void msdl2lyRegularHandler::createInputRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Input group",
        "hinput-group", "help-input-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Input",
        "hinput", "help-input",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("omit-words", subGroup);
  registerAtomInRegularSubgroup ("convert-tempos-to-rehearsal-marks", subGroup);
  registerAtomInRegularSubgroup ("convert-words-to-rehearsal-marks", subGroup);
  registerAtomInRegularSubgroup ("convert-words-to-tempo", subGroup);

  registerAtomInRegularSubgroup ("dal-segno", subGroup);
}

void msdl2lyRegularHandler::createFilesRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Files group",
        "hfiles-group", "help-files-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Files", "hfiles", "help-files",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("output-file-name", subGroup);
  registerAtomInRegularSubgroup ("auto-output-file-name", subGroup);
}

void msdl2lyRegularHandler::createPartsRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Parts group",
        "hparts-group", "help-parts-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Parts",
        "hparts", "help-parts",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("msr-omit-part-id", subGroup);
  registerAtomInRegularSubgroup ("msr-omit-part-name", subGroup);
  registerAtomInRegularSubgroup ("msr-keep-part-id", subGroup);
  registerAtomInRegularSubgroup ("msr-keep-part-name", subGroup);
  registerAtomInRegularSubgroup ("msr-rename-part", subGroup);
}

void msdl2lyRegularHandler::createStavesRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Staves group",
        "hstaves-group",
        "help-staves-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Staves",
        "hstaves", "help-staves",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("create-voices-staff-relative-numbers", subGroup);

  registerAtomInRegularSubgroup ("ledger-lines-color", subGroup);
}

void msdl2lyRegularHandler::createVoicesRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Voices group",
        "hvoices-group", "help-voices-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Voices",
        "hvoices", "help-voices",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("create-single-line-staves-as-rythmic", subGroup);
  registerAtomInRegularSubgroup ("connect-arpeggios", subGroup);
}

void msdl2lyRegularHandler::createTranspositionsRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Transpositions group",
        "htransp-group", "help-transpositions-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Transpositions",
        "hetransp", "help-transpositions",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms
// JMI
}

void msdl2lyRegularHandler::createClefsRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Clefs group",
        "hlefs-group", "help-clefs-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Clefs",
        "hclefs", "help-clefs",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("msr-replace-clef", subGroup);
  registerAtomInRegularSubgroup ("ignore-redundant-clefs", subGroup);
  registerAtomInRegularSubgroup ("comment-clef-changes", subGroup);
}

void msdl2lyRegularHandler::createKeysRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Keys group",
        "hkeys-group", "help-keys-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Keys",
        "hkeys", "help-keys",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("ignore-redundant-keys", subGroup);
}

void msdl2lyRegularHandler::createTimesRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Times group",
        "htimes-group", "help-times-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Times",
        "htimes", "help-times",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("cubase", subGroup);
  registerAtomInRegularSubgroup ("ignore-redundant-times", subGroup);
  registerAtomInRegularSubgroup ("numerical-time", subGroup);
}

void msdl2lyRegularHandler::createRepeatsRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Repeats group",
        "hreps-group", "help-repeats-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Repeats",
        "hreps", "help-repeats",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("create-implicit-initial-repeat-barline", subGroup);
  registerAtomInRegularSubgroup ("keep-repeat-barlines", subGroup);
  registerAtomInRegularSubgroup ("repeat-brackets", subGroup);
  registerAtomInRegularSubgroup ("ignore-repeat-numbers", subGroup);
}

void msdl2lyRegularHandler::createMeasuresRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Measures group",
        "hmeas-group", "help-measures-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Measures",
        "hmeas", "help-measures",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("add-empty-measures", subGroup);
  registerAtomInRegularSubgroup ("replicate-empty-measure", subGroup);
}

void msdl2lyRegularHandler::createRestsRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Rests group",
        "hrests-group", "help-rests-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Rests",
        "hrests", "help-rests",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("delay-rests-slashes", subGroup);

  registerAtomInRegularSubgroup ("compress-full-measure-rests", subGroup);
  registerAtomInRegularSubgroup ("merge-rests", subGroup);
}

void msdl2lyRegularHandler::createNotesRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Notes group",
        "hnotes-group", "help-notes-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Notes",
        "hnotes", "help-notes",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("msr-pitches-language", subGroup);

  registerAtomInRegularSubgroup ("absolute", subGroup);
  registerAtomInRegularSubgroup ("relative", subGroup);
  registerAtomInRegularSubgroup ("fixed", subGroup);

  registerAtomInRegularSubgroup ("white-note-heads", subGroup);
  registerAtomInRegularSubgroup ("generate-commented-out-variables", subGroup);

  registerAtomInRegularSubgroup ("accidental-style", subGroup);

  registerAtomInRegularSubgroup ("non-print-notes-head-color", subGroup);
}

void msdl2lyRegularHandler::creatBeamsRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Beams group",
        "hbeams-group", "help-beams-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Beams",
        "hbeams", "help-beams",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("no-auto-beaming", subGroup);
  registerAtomInRegularSubgroup ("no-beams", subGroup);

  registerAtomInRegularSubgroup ("delay-rests-beams", subGroup); // JMI
}

void msdl2lyRegularHandler::createArticulationsRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Articulations group",
        "harts-group", "help-articulations-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Articulations",
        "harts", "help-articulations",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("omit-articulations", subGroup);

  registerAtomInRegularSubgroup ("delay-rests-pedals", subGroup);
}

void msdl2lyRegularHandler::createOrnamentsRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Ornaments group",
        "horns-group", "help-ornaments-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Ornaments",
        "horns", "help-ornaments",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("omit-ornaments", subGroup);

  registerAtomInRegularSubgroup ("delayed-ornaments-fraction", subGroup);
}

void msdl2lyRegularHandler::createGraceNotesRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Grace notes group",
        "hgraces-group", "help-grace-notes-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Grace notes",
        "hgraces", "help-grace-notes",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("slash-all-grace-notes", subGroup);
  registerAtomInRegularSubgroup ("slur-all-grace-notes", subGroup);
  registerAtomInRegularSubgroup ("beam-all-grace-notes", subGroup);
}

void msdl2lyRegularHandler::createChordsRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Chords group",
        "hchords-group", "help-chords-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Chords",
        "hchords", "help-chords",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

#ifdef TRACING_IS_ENABLED
  registerAtomInRegularSubgroup ("trace-chords", subGroup);
#endif

  registerAtomInRegularSubgroup ("lpsr-pitches-language", subGroup);

  registerAtomInRegularSubgroup ("lpsr-chords-language", subGroup);

  registerAtomInRegularSubgroup ("chords-display", subGroup);
}

void msdl2lyRegularHandler::createTiesRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Ties group",
        "hties-group", "help-ties-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Ties",
        "hties", "help-ties",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("omit-ties", subGroup);
}

void msdl2lyRegularHandler::createSlursRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Slurs group",
        "hslurs-group", "help-slurs-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Slurs",
        "hslurs", "help-slurs",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("omit-slurs", subGroup);

  registerAtomInRegularSubgroup ("delay-rests-slurs", subGroup);
}

void msdl2lyRegularHandler::createLigaturesRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Ligatures group",
        "hligs-group", "help-ligatures-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Ligatures",
        "hligs", "help-ligatures",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("delay-rests-ligatures", subGroup);
}

void msdl2lyRegularHandler::createDynamicsRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Dynamics group",
        "hdyns-group", "help-dynamics-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Dynamics",
        "hdyns", "help-dynamics",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("omit-dynamics", subGroup);

  registerAtomInRegularSubgroup ("all-dynamics-below", subGroup);

  registerAtomInRegularSubgroup ("delay-rests-dynamics", subGroup);
}

void msdl2lyRegularHandler::createWedgesRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Wedges group",
        "hweds-group", "help-wedges-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Wedges",
        "hweds", "help-wedges",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("omit-wedges", subGroup);

  registerAtomInRegularSubgroup ("all-wedges-below", subGroup);

  registerAtomInRegularSubgroup ("delay-rests-wedges", subGroup);
}

void msdl2lyRegularHandler::createTupletsRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Tuplets group",
        "htups-group", "help-tuplets-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Tuplets",
        "htups", "help-tuplets",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("indent-tuplets", subGroup);
}

void msdl2lyRegularHandler::createStringsRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Strings group",
        "hstrings-group", "help-strings-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Strings",
        "hstrings", "", "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("roman-string-numbers", subGroup);
  registerAtomInRegularSubgroup ("avoid-open-strings", subGroup);
}

void msdl2lyRegularHandler::createTablaturesRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Tablatures group",
        "htabs-group", "help-tablatures-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Tablatures",
        "htabs", "help-tablatures",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("tabFullNotation", subGroup);
  registerAtomInRegularSubgroup ("modernTab", subGroup);
}

void msdl2lyRegularHandler::createHarmoniesRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Harmonies group",
        "hharms-group", "help-harmonies-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Harmonies",
        "hharms", "help-harmonies",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("omit-harmonies", subGroup);

  registerAtomInRegularSubgroup ("show-harmony-voices", subGroup);

#ifdef EXTRA_OAH_IS_ENABLED
  registerAtomInRegularSubgroup ("show-harmonies-structures", subGroup);
  registerAtomInRegularSubgroup ("show-all-harmonies-contents", subGroup);
  registerAtomInRegularSubgroup ("show-harmony-details", subGroup);
  registerAtomInRegularSubgroup ("show-harmony-analysis", subGroup);
#endif
}

void msdl2lyRegularHandler::createFiguredBassesRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Figured basses group",
        "hfigbass-group", "help-figured-basses-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Figured basses",
        "hfigbass", "help-figured-basses",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("omit-figured-basses", subGroup);

  registerAtomInRegularSubgroup ("show-figured-bass-voices", subGroup);
}

void msdl2lyRegularHandler::createLyricsRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Lyrics group",
        "hlyrics-group", "help-lyrics-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Lyrics",
        "hlyrics", "help-lyrics",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("omit-lyrics", subGroup);
  registerAtomInRegularSubgroup ("add-words-from-the-lyrics", subGroup);

  registerAtomInRegularSubgroup ("no-lilypond-lyrics", subGroup);
  registerAtomInRegularSubgroup ("lyrics-durations", subGroup);
  registerAtomInRegularSubgroup ("add-stanzas-numbers-to-lyrics", subGroup);
}

void msdl2lyRegularHandler::createBookRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Book group",
        "hbook-group", "help-book-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Book",
        "hbook", "help-book",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("no-book-block", subGroup);
  registerAtomInRegularSubgroup ("no-header-block", subGroup);
  registerAtomInRegularSubgroup ("no-paper-block", subGroup);
  registerAtomInRegularSubgroup ("no-bar-number-checks", subGroup);
}

void msdl2lyRegularHandler::createHeaderRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Header group",
        "hheader-group", "help-header-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Header",
        "hheader", "help-header",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("rights", subGroup);

  registerAtomInRegularSubgroup ("composer", subGroup);
  registerAtomInRegularSubgroup ("arranger", subGroup);
  registerAtomInRegularSubgroup ("lyricist", subGroup);
  registerAtomInRegularSubgroup ("poet", subGroup);

  registerAtomInRegularSubgroup ("software", subGroup);

  registerAtomInRegularSubgroup ("dedication", subGroup);
  registerAtomInRegularSubgroup ("piece", subGroup);
  registerAtomInRegularSubgroup ("opus", subGroup);

  registerAtomInRegularSubgroup ("title", subGroup);
  registerAtomInRegularSubgroup ("subtitle", subGroup);
  registerAtomInRegularSubgroup ("subsubtitle", subGroup);

  registerAtomInRegularSubgroup ("instrument", subGroup);

  registerAtomInRegularSubgroup ("meter", subGroup);

  registerAtomInRegularSubgroup ("copyright", subGroup);

  registerAtomInRegularSubgroup ("tagline", subGroup);

  registerAtomInRegularSubgroup ("use-lyricists-as-poets", subGroup);

  registerAtomInRegularSubgroup ("use-filename-as-work-title", subGroup);
}

void msdl2lyRegularHandler::createPaperRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Paper group",
        "h-group", "help-paper-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Paper",
        "hpaper", "help-paper",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("paper-height", subGroup);
  registerAtomInRegularSubgroup ("paper-width", subGroup);

  registerAtomInRegularSubgroup ("left-margin", subGroup);
  registerAtomInRegularSubgroup ("right-margin", subGroup);
  registerAtomInRegularSubgroup ("top-margin", subGroup);
  registerAtomInRegularSubgroup ("bottom-margin", subGroup);

  registerAtomInRegularSubgroup ("ragged-bottom", subGroup);
  registerAtomInRegularSubgroup ("ragged-last", subGroup);
  registerAtomInRegularSubgroup ("ragged-last-bottom", subGroup);
  registerAtomInRegularSubgroup ("ragged-right", subGroup);

  registerAtomInRegularSubgroup ("horizontal-shift", subGroup);
  registerAtomInRegularSubgroup ("indent", subGroup);
  registerAtomInRegularSubgroup ("short-indent", subGroup);

  registerAtomInRegularSubgroup ("markup-system-spacing-padding", subGroup);

  registerAtomInRegularSubgroup ("page-count", subGroup);
  registerAtomInRegularSubgroup ("system-count", subGroup);
}

void msdl2lyRegularHandler::createLayoutRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Layout group",
        "hlayout-group", "help-layout-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Layout",
        "hlayout", "help-layout",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("global-staff-size", subGroup);
  registerAtomInRegularSubgroup ("keep-staff-size", subGroup);
}

void msdl2lyRegularHandler::createOutputRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "Ouptut group",
        "houptut-group", "help-ouptut-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "Ouptut",
        "houptut", "help-ouptut",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("msdl2ly-infos", subGroup);

  registerAtomInRegularSubgroup ("no-lilypond-code", subGroup);

  registerAtomInRegularSubgroup ("draft-mode", subGroup);
  registerAtomInRegularSubgroup ("point-and-clic-off", subGroup);

  registerAtomInRegularSubgroup ("lilypond-version", subGroup);
  registerAtomInRegularSubgroup ("lilypond-compile-date", subGroup);
  registerAtomInRegularSubgroup ("lilypond-comments", subGroup);

  registerAtomInRegularSubgroup ("no-set-global-staff-size", subGroup);

  registerAtomInRegularSubgroup ("msr2lpsr-score-output-kind", subGroup);

  registerAtomInRegularSubgroup ("global", subGroup);

  registerAtomInRegularSubgroup ("ambitus", subGroup);

  registerAtomInRegularSubgroup ("input-line-numbers", subGroup);

  registerAtomInRegularSubgroup ("generate-moments-in-measures", subGroup);
  registerAtomInRegularSubgroup ("generate-positions-in-measures", subGroup);

  registerAtomInRegularSubgroup ("all-bar-numbers", subGroup);
  registerAtomInRegularSubgroup ("all-measure-numbers", subGroup);
  registerAtomInRegularSubgroup ("generate-box-around-bar-number", subGroup);
  registerAtomInRegularSubgroup ("reset-measure-number", subGroup);
  registerAtomInRegularSubgroup ("original-measure-numbers", subGroup);

  registerAtomInRegularSubgroup ("separator-line-every-n-measures", subGroup);

  registerAtomInRegularSubgroup ("ignore-musicxml-line-breaks", subGroup);
  registerAtomInRegularSubgroup ("suppress-musicxml-line-breaks", subGroup);

  registerAtomInRegularSubgroup ("ignore-musixcml-page-breaks", subGroup);
  registerAtomInRegularSubgroup ("suppress-musixcml-page-breaks", subGroup);

  registerAtomInRegularSubgroup ("break-lines-at-incomplete-right-measures", subGroup);
  registerAtomInRegularSubgroup ("custos", subGroup);

  registerAtomInRegularSubgroup ("length-unit", subGroup);

  registerAtomInRegularSubgroup ("all-durations", subGroup);

  registerAtomInRegularSubgroup ("jazz-fonts", subGroup);

  registerAtomInRegularSubgroup ("jianpu", subGroup);
  registerAtomInRegularSubgroup ("lyluatex", subGroup);

  registerAtomInRegularSubgroup ("minimal", subGroup);
}

void msdl2lyRegularHandler::createMidiRegularGroup ()
{
  // group

  S_oahGroup
    group =
      oahGroup::create (
        "MIDI group",
        "hmidi-group", "help-midi-group",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole);
  appendGroupToRegulalHandler (group);

  // subgroup

  S_oahSubGroup
    subGroup =
      oahSubGroup::create (
        "MIDI",
        "hmidi", "help-midi",
        "",
        oahElementVisibilityKind::kElementVisibilityWhole,
        group);
  group->
    appendSubGroupToGroup (subGroup);

  // atoms

  registerAtomInRegularSubgroup ("no-midi", subGroup);
  registerAtomInRegularSubgroup ("midi-tempo", subGroup);
}

//______________________________________________________________________________
void msdl2lyRegularHandler::checkOptionsAndArgumentsFromArgcAndArgv () const
{
#ifdef TRACING_IS_ENABLED
  if (getTraceOah ()) {
    gLogStream <<
      "checking options and arguments from argc/argv in \"" <<
      fHandlerHeader <<
      "\"" <<
      endl;
  }
#endif

  checkSingleInputSourceInArgumentsVector ();
}

//______________________________________________________________________________
void msdl2lyRegularHandler::print (ostream& os) const
{
  const unsigned int fieldWidth = 27;

  os <<
    "msdl2lyRegularHandler '" << fHandlerHeader << "':" <<
    endl;

  ++gIndenter;

  printHandlerEssentials (
    os, fieldWidth);
  os << endl;

  os <<
    "Options groups (" <<
    singularOrPlural (
      fHandlerGroupsList.size (), "element",  "elements") <<
    "):" <<
    endl;

  if (fHandlerGroupsList.size ()) {
    os << endl;

    ++gIndenter;

    list<S_oahGroup>::const_iterator
      iBegin = fHandlerGroupsList.begin (),
      iEnd   = fHandlerGroupsList.end (),
      i      = iBegin;
    for ( ; ; ) {
      // print the options group
      os << (*i);
      if (++i == iEnd) break;
      os << endl;
    } // for

    --gIndenter;
  }

  --gIndenter;

  os << endl;
}

ostream& operator<< (ostream& os, const S_msdl2lyRegularHandler& elt)
{
  elt->print (os);
  return os;
}


}
