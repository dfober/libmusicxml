/*
  MusicXML Library
  Copyright (C) Grame 2006-2013

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr
*/

#ifndef ___bsrTranscriptionNotes___
#define ___bsrTranscriptionNotes___

#include "bsrElements.h"
#include "bsrLines_MUT_DEP.h"


namespace MusicXML2
{

//______________________________________________________________________________
class EXP bsrTranscriptionNotesElement : public bsrElement
{
  public:

    // creation
    // ------------------------------------------------------

    static SMARTP<bsrTranscriptionNotesElement> create (
      int    inputLineNumber,
      string transcriptionNoteText);

  protected:

    // constructors/destructor
    // ------------------------------------------------------

    bsrTranscriptionNotesElement (
      int    inputLineNumber,
      string transcriptionNoteText);

    virtual ~bsrTranscriptionNotesElement ();

  public:

    // set and get
    // ------------------------------------------------------

    string                getTranscriptionNoteText () const
                              { return fTranscriptionNoteText; }

  public:

    // public services
    // ------------------------------------------------------

    S_bsrCellsList        fetchCellsList () const
                              { return buildCellsList (); }

  private:

    // private services
    // ------------------------------------------------------

    S_bsrCellsList        buildCellsList () const;

  public:

    // visitors
    // ------------------------------------------------------

    virtual void          acceptIn  (basevisitor* v) override;
    virtual void          acceptOut (basevisitor* v) override;

    virtual void          browseData (basevisitor* v) override;

  public:

    // print
    // ------------------------------------------------------

    string                asString () const override;

    virtual void          print (ostream& os) const override;

  private:

    // private fields
    // ------------------------------------------------------

    string                fTranscriptionNoteText;
};
typedef SMARTP<bsrTranscriptionNotesElement> S_bsrTranscriptionNotesElement;
EXP ostream& operator<< (ostream& os, const S_bsrTranscriptionNotesElement& elt);

//______________________________________________________________________________
class EXP bsrTranscriptionNotes : public bsrLine
{
  public:

    // creation
    // ------------------------------------------------------

    static SMARTP<bsrTranscriptionNotes> create (
      int inputLineNumber);

  protected:

    // constructors/destructor
    // ------------------------------------------------------

    bsrTranscriptionNotes (
      int inputLineNumber);

    virtual ~bsrTranscriptionNotes ();

  public:

    // set and get
    // ------------------------------------------------------

  public:

    // public services
    // ------------------------------------------------------

    void                  appendElementToTranscriptionNotes (
                            S_bsrTranscriptionNotesElement transcriptionNotesElement)
                              {
                                fTranscriptionNotesElementsList.push_back (
                                  transcriptionNotesElement);
                              }

    int                   fetchLineContentsNumber () const override
                              { return fLineContentsList.size (); }

  public:

    // visitors
    // ------------------------------------------------------

    virtual void          acceptIn  (basevisitor* v) override;
    virtual void          acceptOut (basevisitor* v) override;

    virtual void          browseData (basevisitor* v) override;

  public:

    // print
    // ------------------------------------------------------

    virtual void          print (ostream& os) const override;

  private:

    // private fields
    // ------------------------------------------------------

    list<S_bsrTranscriptionNotesElement>
                          fTranscriptionNotesElementsList;
};
typedef SMARTP<bsrTranscriptionNotes> S_bsrTranscriptionNotes;
EXP ostream& operator<< (ostream& os, const S_bsrTranscriptionNotes& elt);


} // namespace MusicXML2


#endif
