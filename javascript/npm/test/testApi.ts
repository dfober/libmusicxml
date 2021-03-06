
///<reference path="../libmusicxml.ts"/>

//----------------------------------------------------------------------------
interface TLog { (text: string): void; }

//----------------------------------------------------------------------------
// Misc. functions
//----------------------------------------------------------------------------
function misc (lib: libmusicxml, log: TLog )
{
	log( "Version :");
	log ("  libmusicxml version        " + lib.libVersion());
	log ("  libmusicxml version str    " + lib.libVersionStr());
	log ("  musicxml2guido version     " + lib.musicxml2guidoVersion());
	log ("  musicxml2guido version str " + lib.musicxml2guidoVersionStr());
}

function convert (xml: string, lib: libmusicxml, log: TLog )
{
	log( "\nMusicXML -> GMN :");
	log ("  string2guido  :")
	log (lib.string2guido (xml, false));
	log ("  xmlStringTranspose :");
	log (lib.string2guido (lib.xmlStringTranspose(xml, 7), false));
}


function run (engine: libmusicxml, log: TLog, xml: string) {
 	misc 	(engine, log);
	if (xml) convert (xml, engine, log );
}

var process: any;
var module: any;
if ((typeof process !== 'undefined') && (process.release.name === 'node')) {
	module.exports = run;
}
