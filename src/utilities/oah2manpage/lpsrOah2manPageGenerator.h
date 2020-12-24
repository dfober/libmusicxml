/*
  MusicXML Library
  Copyright (C) Grame 2006-2013

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr
*/

#ifndef ___lpsrOah2manPageGenerator___
#define ___lpsrOah2manPageGenerator___

#include "lpsrOah.h"

#include "oah2manPageGenerators.h"


namespace MusicXML2
{

//________________________________________________________________________
class lpsrOah2manPageGenerator : virtual public oah2manPageGenerator,

// JMI  public visitor<S_lpsrScoreOutputKindAtom>,

  public visitor<S_lpsrPitchesLanguageAtom>,

  public visitor<S_lpsrChordsLanguageAtom>,

  public visitor<S_lpsrTransposeAtom>

{
  public:

    lpsrOah2manPageGenerator (
      const S_oahHandler handler,
      ostream&           manPageOutputStream);

    virtual ~lpsrOah2manPageGenerator ();

  protected:

/* JMI
    virtual void visitStart (S_lpsrScoreOutputKindAtom& elt);
    virtual void visitEnd   (S_lpsrScoreOutputKindAtom& elt);
*/
    virtual void visitStart (S_lpsrPitchesLanguageAtom& elt);
    virtual void visitEnd   (S_lpsrPitchesLanguageAtom& elt);

    virtual void visitStart (S_lpsrChordsLanguageAtom& elt);
    virtual void visitEnd   (S_lpsrChordsLanguageAtom& elt);

    virtual void visitStart (S_lpsrTransposeAtom& elt);
    virtual void visitEnd   (S_lpsrTransposeAtom& elt);
};
typedef SMARTP<lpsrOah2manPageGenerator> S_lpsrOah2manPageGenerator;


}


#endif
