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
  , --filter-opts <arg>  | Filter options
-s, --rootstart <arg>    | Define the root start node
-e, --jumpstart <arg>    | Define the root end node
-n, --nodes <arg>        | Define node key-value pairs (separated by comma)
  , --nodefile <arg>     | Define node key-value pairs from a file
-k, --show-full-key      | Show a full key
  , --see-raw-html <arg> | Dump received HTML to file.
  , --see-parsed-html    | Dump the parsed HTML and stop.
  , --see-parsed-lua     | Dump the parsed Lua file and stop.
  , --see-crude-frames   | Show the frames at first pass
  , --see-nice-frames    | Show the frames at second pass
  , --step               | Step through frames
  , --mysql              | Choose MySQL as the SQL template.
  , --pgsql              | Choose PostgreSQL as the SQL template.
  , --mssql              | Choose SQL Server as the SQL template.
-v, --verbose            | Say more than usual
-h, --help               | Show help


## Why would I want this?

You would want to use baht if you need to write a script that can harvest data.  Web scrapers can be challenging and time-consuming to architect, and this tool can make your life easier. 


## Details

Baht is very early on in its development, and is being modified somewhat irregularly. 
Additional documentation and tutorials are soon to follow.


