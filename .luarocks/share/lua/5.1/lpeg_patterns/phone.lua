-- Phone numbers

local lpeg = require "lpeg"
local P = lpeg.P
local R = lpeg.R
local S = lpeg.S

local digit = R"09"
local seperator = S"- ,."
local function optional_parens(patt)
	return P"(" * patt * P")" + patt
end

local _M = {}

local extension = P"e" * (P"xt")^-1 * seperator^-1 * digit^1
local optional_extension = (seperator^-1 * extension)^-1

_M.Australia = (
	-- Normal landlines
	optional_parens((P"0")^-1*S"2378") * seperator^-1 * digit*digit*digit*digit * seperator^-1 * digit*digit*digit*digit
	-- Mobile numbers
	+ (optional_parens(P"0"*S"45"*digit*digit) + S"45"*digit*digit)
		* seperator^-1 * digit*digit*digit * seperator^-1 * digit*digit*digit
	-- Local rate calls
	+ P"1300" * seperator^-1 * digit*digit*digit * seperator^-1 * digit*digit*digit
	-- 1345 is only used for back-to-base monitored alarm systems
	+ P"1345" * seperator^-1 * digit*digit * seperator^-1 * digit*digit
	+ P"13"   * seperator^-1 * digit*digit * seperator^-1 * digit*digit
	+ (P"0")^-1*P"198" * seperator^-1 * digit*digit*digit * seperator^-1 * digit*digit*digit -- data calls
	-- Free calls
	+ P"1800" * seperator^-1 * digit*digit*digit * seperator^-1 * digit*digit*digit
	+ P"180"  * seperator^-1 * digit*digit*digit*digit
) * optional_extension

local NPA = (digit-S"01")*digit*digit
local NXX = ((digit-S"01")*(digit-P"9")-P"37"-P"96")*digit-P(1)*P"11"
local USSubscriber = digit*digit*digit*digit
_M.USA = ((P"1" * seperator^-1)^-1 * optional_parens(NPA) * seperator^-1)^-1
	* NXX * seperator^-1 * USSubscriber * optional_extension

local international = (
	P"1" * seperator^-1 * #(-P"1") * _M.USA
	+ P"61" * seperator^-1 * #(digit-P"0") * _M.Australia
	-- Other countries we haven't made specific patterns for yet
	+(P"20"+P"212"+P"213"+P"216"+P"218"+P"220"+P"221"
	+P"222"+P"223"+P"224"+P"225"+P"226"+P"227"+P"228"+P"229"
	+P"230"+P"231"+P"232"+P"233"+P"234"+P"235"+P"236"+P"237"
	+P"238"+P"239"+P"240"+P"241"+P"242"+P"243"+P"244"+P"245"
	+P"246"+P"247"+P"248"+P"249"+P"250"+P"251"+P"252"+P"253"
	+P"254"+P"255"+P"256"+P"257"+P"258"+P"260"+P"261"+P"262"
	+P"263"+P"264"+P"265"+P"266"+P"267"+P"268"+P"269"+P"27"
	+P"290"+P"291"+P"297"+P"298"+P"299"+P"30" +P"31" +P"32"
	+P"33" +P"34" +P"350"+P"351"+P"352"+P"353"+P"354"+P"355"
	+P"356"+P"357"+P"358"+P"359"+P"36" +P"370"+P"371"+P"372"
	+P"373"+P"374"+P"375"+P"376"+P"377"+P"378"+P"380"+P"381"
	+P"385"+P"386"+P"387"+P"389"+P"39" +P"40" +P"41" +P"420"
	+P"421"+P"423"+P"43" +P"44" +P"45" +P"46" +P"47" +P"48"
	+P"49" +P"500"+P"501"+P"502"+P"503"+P"504"+P"505"+P"506"
	+P"507"+P"508"+P"509"+P"51" +P"52" +P"53" +P"54" +P"55"
	+P"56" +P"57" +P"58" +P"590"+P"591"+P"592"+P"593"+P"594"
	+P"595"+P"596"+P"597"+P"598"+P"599"+P"60" +P"62"
	+P"63" +P"64" +P"65" +P"66" +P"670"+P"672"+P"673"+P"674"
	+P"675"+P"676"+P"677"+P"678"+P"679"+P"680"+P"681"+P"682"
	+P"683"+P"684"+P"685"+P"686"+P"687"+P"688"+P"689"+P"690"
	+P"691"+P"692"+P"7"  +P"808"+P"81" +P"82" +P"84" +P"850"
	+P"852"+P"853"+P"855"+P"856"+P"86" +P"870"+P"871"+P"872"
	+P"873"+P"874"+P"878"+P"880"+P"881"+P"886"+P"90" +P"91"
	+P"92" +P"93" +P"94" +P"95" +P"960"+P"961"+P"962"+P"963"
	+P"964"+P"965"+P"966"+P"967"+P"968"+P"970"+P"971"+P"972"
	+P"973"+P"974"+P"975"+P"976"+P"977"+P"98" +P"992"+P"993"
	+P"994"+P"995"+P"996"+P"998" ) * (seperator^-1*digit)^6 -- At least 6 digits
)

_M.phone = P"+" * seperator^-1 * international

return _M
