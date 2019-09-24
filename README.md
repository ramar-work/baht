<pre>
'''''',,,,,,,,,,,,;;;;;;;;;;;;:ccloodddddl:;;;,,,,,,,,,,,,,,,,,,,;;;;;;:::coOXX0
'''',,,,,,,,,,,;;;;;;;;;:::dkxdddddoododdk0Kkc;,;,,,,,,,,,,,,,,,,,;;;::::cdNMMMM
,,,,,,,,,,,,;;;;;;;;;::::dOxdooooooooooooooxOKOc;;,,,,,,,,;::::loolxKNNKkoxWMMMM
,,,,,,,;,;;;;;;;;;:::::l00xdoooolllloooodxxddddxl,,,,,,,;cllldkkkxONNNNNNXkkXWMM
,,,,,,,,,,;;;;;;:clllldK0xdoolccllllllcccccccccloc,,,;;;:llooxkkkxkKNNNNNNOxdxkx
,,,,,,,,,,,,;;;:loodddOkxdolllcllcc:;;,;;;:;;:cccl:;;;:::cloooodxxdolxOOOxdoddoo
,,,,,,,,,,,,,;;cloddxkxdololccccll:;;;,,,,,,;::llcl::::::::::::;;,,,,;;::cloodoo
,,,,,,,,,,,,,;;cdxkOkl;c:,.......':c:cc:;;,''',;c::::::::::;;;,,,,,,,;;;::cllooo
,,,,,,,,,,,,;:cdkO00O'..c;.. .;.''';:c::;,.';.''.;:::::;;;;,,,,,,,,,,;;;::::ccll
,,,,,,,,,,;;:coxkO00K0lxxollllloddolc:;;,'',...,';c;;;;;,,,,,,,,,,,,,;;;;;::::::
,,,,,,,,,;;:lodxxkOOOOOkoll::cc:;;;,,,,,,,..',,,:oc;;,,,,,,,,,,,,,,,;;;;;;;;;;;;
',,,,,,,;;:clodxxkkkkkdc;,,;';;,'..'''.'''.',',ldoo;,,,,,,,,,,,,,,,,,;;;;;;;;;;,
,,,,,,,;::cloddxxxxxxxdl;',:llc;,''..........';clodl,,,,,''',,,,,,,,,,;;;;;;;,,,
',,,,;;::cllodddxxxxxxoc:,'',,,,,'..'......'::;;:lxkl,,,,',,,,,,,,,,,,,;;;;,,,,,
,,,,;;::clloooddddddddo':::;''.........  ..,;;;;:::ldxxddxkkd:,,,,,,,,,,,,,,,,,,
,,;;;::okOkkxxddddddddddol:,.......     ....';;,;:clllllooddOXXOo;,,,,,,,,,,,,,,
,;;;lkK0OxxdddddodOOkxooc;,''......     ......',;lllcllooodddxx0XXOd;,,,,,,,,,,'
,;o00kxxoxdddddooddKXXK0xl;,''...       ......',;coolllllodddoooxOOKX0d;,,,,,'''
:kOolc:;llooddddxxx0KKKKX0o;'',,;::'.........'';:lollooodddddoooxkxk0KKKx:,','''
ol;,''.,',;:cloooodx0KOOO0XKdcc::::;'.......',:cllldddddddooooddxxxxxkkkkxl,''''
c;'...,,,,,,;;;:cccccldxkO0KNKo::::;;.....,;:clooodxxxxxxxdddoooooooooooolol''''
:,,'.....,;;;;,;;;;:::clod0XKKKx:::okx.;dOOxdoolloodddddddxdolllcccccc::::cc;'''
cc'..... ..;,,,,,,,;;:::ccloO0ddcd0KKkldkxxkkOkodccclloooloollcc:::::;;;;;;;l,''
N0l,......,lc,'',,,,,,;:::::;xOOO0kO0KxxxddkkOOxoc;::cllllllc::::::;;;,,,;,,co''
0xxxolcoddollc;,,,'';;;,,,cx0kkxO00xxxddxxxxkkkxl,.;lodooolc::::::::;;;,;;';cl:.
xxxxxxddl::::::;;,',,,''cx0KKXKkO0KXOlllddxxxxxdl;.ldddddoollccllodol::;;;',::c'
dooooll:;;;;;;,,'.'.;;ldxk00KKXXXKXKKKOdddddddolc:oddddddooollllodxxdl:::,,;;;:,
::::::;,,,,,''''...';;:cdkOOO0KXXKKXXK0x::ooolc::lddodddooooloooooxxdoc:;,,;;;:,
''',,'''......','..,;;;;;;xOOO00KKXOo;.'';cc:::::odoooooooolllooodxxocc:;,,;::c;
'.........'':'.....',,,,,,:lxkO000d. .:clc:::;;,;ccc::cllllllllllool::;:,',;;:cc
,'..........:,. ...o:,,,,,,,,lkkd,  .';cclllcc:::::;;:::ccccccloolc;'',,'.',;;::
,,,,'........    ..oo;,,,,,,,,oc. .....,;:cclccccccccc:::;;coddolc;,''''..',,,;;
</pre>


# baht

A tool for gathering information from the internet.


## Usage 

Option | Description
------ | -----------
-l, --load <arg>         | Load a file on the command line.
-f, --file <arg>         | Get a file on the command line.
-u, --url <arg>          | Check a URL from the WWW
-o, --output <arg>       | Send output to this file
-t, --tmp <arg>          | Use this as a source folder for any downloads.
    --filter-opts <arg>  | Filter options
-s, --rootstart <arg>    | Define the root start node
-e, --jumpstart <arg>    | Define the root end node
-n, --nodes <arg>        | Define node key-value pairs (separated by comma)
    --nodefile <arg>     | Define node key-value pairs from a file
-k, --show-full-key      | Show a full key
    --see-raw-html <arg> | Dump received HTML to file.
    --see-parsed-html    | Dump the parsed HTML and stop.
    --see-parsed-lua     | Dump the parsed Lua file and stop.
    --see-crude-frames   | Show the frames at first pass
    --see-nice-frames    | Show the frames at second pass
    --step               | Step through frames
    --mysql              | Choose MySQL as the SQL template.
    --pgsql              | Choose PostgreSQL as the SQL template.
    --mssql              | Choose SQL Server as the SQL template.
-v, --verbose            | Say more than usual
-h, --help               | Show help


## Setup

`make build` and `make install` will get you going.


## Why would I want this?

You would want to use baht if you need to write a script that can harvest data.  
Web scrapers can be challenging and time-consuming to architect, and this tool can make your life easier. 


## Details

Baht is very early on in its development, and is being modified somewhat irregularly. 
Additional documentation and tutorials are soon to follow.


## Contact

- ramar@tubularmodular.com


<link rel="stylesheet" href="style.css">
