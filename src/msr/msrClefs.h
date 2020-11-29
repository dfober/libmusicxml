/*
  MusicXML Library
  Copyright (C) Grame 2006-2013

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr
*/

#ifndef ___msrClefs___
#define ___msrClefs___

#include "msrSegnosAndCodas.h"

#include "msrBasicTypes.h"


namespace MusicXML2
{

//______________________________________________________________________________
class msrClef;
typedef SMARTP<msrClef> S_msrClef;

class msrClef : public msrMeasureElement
{
  public:

    // creation from MusicXML
    // ------------------------------------------------------

    static SMARTP<msrClef> create (
      int         inputLineNumber,
      msrClefKind clefKind,
      int         clefStaffNumber);

  protected:

    // constructors/destructor
    // ------------------------------------------------------

    msrClef (
      int         inputLineNumber,
      msrClefKind clefKind,
      int         clefStaffNumber);

    virtual ~msrClef ();

  public:

    // set and get
    // ------------------------------------------------------

    msrClefKind           getClefKind () const
                              { return fClefKind; }

    int                   getClefStaffNumber () const
                              { return fClefStaffNumber; }

  public:

    // public services
    // ------------------------------------------------------

    bool                  isEqualTo (S_msrClef otherClef) const;

    bool                  clefIsATablatureClef () const;

    bool                  clefIsAPercussionClef () const;

  public:

    // visitors
    // ------------------------------------------------------

    virtual void          acceptIn  (basevisitor* v) override;
    virtual void          acceptOut (basevisitor* v) override;

    virtual void          browseData (basevisitor* v) override;

  public:

    // public services
    // ------------------------------------------------------

  public:

    // print
    // ------------------------------------------------------

    string                asString () const override;

    virtual void          print (ostream& os) const override;

  private:

    // private fields
    // ------------------------------------------------------

    msrClefKind           fClefKind;
    int                   fClefStaffNumber;
};
typedef SMARTP<msrClef> S_msrClef;
EXP ostream& operator<< (ostream& os, const S_msrClef& elt);


} // namespace MusicXML2


#endif
