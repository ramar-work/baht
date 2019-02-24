Nodeblock nodes[] = {
/*.content = "files/carri.html"*/
	{
		.rootNode = {.string = "div^backdrop.div^content_a.div^content_b.center"} 
	 //,.jumpNode = { .string = "div^backdrop.div^content_a.div^content_b.center" } 
	 ,.jumpNode = {.string = "div^backdrop.div^content_a.div^content_b.center.div^thumb_div" }

	//,.deathNode = {.string= "div^backdrop.div^content_a.div^content_b.center.br" }
	//,.miniDeathNode = {.string="div^backdrop.div^content_a.div^content_b.center.input#raw_price_13" }
	}
};


//Hash this somewhere.
yamlList expected_keys[] = {
   { "model" }
  ,{ "year" }
  ,{ "make" }
  ,{ "engine" }
  ,{ "mileage" }
  ,{ "transmission" }
  ,{ "drivetrain" }
  ,{ "abs" }
  ,{ "air_conditioning" }
  ,{ "carfax" }
  ,{ "autocheck" }
  ,{ "autotype" }
  ,{ "price" }
  ,{ "fees" }
  ,{ "kbb" }
  ,{ "engine" }
  ,{ "mpg" }
  ,{ "fuel_type" }
  ,{ "interior" }
  ,{ "exterior" }
  ,{ "vin" }
  ,{ NULL }
};

yamlList testNodes[] = {
 {           "model", "div^thumb_div.table^thumb_table_a.tbody.tr.td^thumb_ymm.text" }
,{ "individual_page", "div^thumb_div.table^thumb_table_a.tbody.tr.td^font5 thumb_more_info.a^font6 thumb_more_info_link.attrs.href" }
,{         "mileage", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.text" }
,{    "transmission", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr1.td.text" }
,{           "price", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr1.td1.text" }
,{          "engine", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td1.text" }
,{     "description", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr2.td.p^font3 thumb_description_text.text" }
,{              NULL, NULL }
};

#if 0
yamlList testNodes[] = {
   {              "model", "div^thumb_div.table^thumb_table_a.tbody.tr.td^thumb_ymm.text" }
#if 1 
  ,{    "individual_page", "div^thumb_div.table^thumb_table_a.tbody.tr.td^font5 thumb_more_info.a^font6 thumb_more_info_link.attrs.href" }
  ,{            "mileage", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.1.tr.2.td.text" }
  ,{       "transmission", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.2.tr.1.td.text" }
  ,{              "price", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.2.tr.2.td.text" }
#else
  ,{               "year", NULL }
  ,{               "make", NULL }
  ,{    "individual_page", "div^thumb_div.table^thumb_table_a.tbody.tr.td^font5 thumb_more_info.a^font6 thumb_more_info_link.attrs.href" }
  ,{            "mileage", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.text" }
  ,{       "transmission", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.text" }
  ,{         "drivetrain", NULL }
  ,{                "abs", NULL }
  ,{   "air_conditioning", NULL }
  ,{             "carfax", NULL }
  ,{          "autocheck", NULL }
  ,{           "autotype", NULL }
  ,{              "price", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.text" }
  ,{               "fees", NULL }
  ,{                "kbb", NULL }
  ,{        "description", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.p^font3 thumb_description_text.text" }
  ,{             "engine", "div^thumb_div.table^thumb_table_b.tbody.tr.td^thumb_content_right.div^thumb_table_c.table^font7 thumb_info_text.tbody.tr.td.text" }
  ,{                "mpg", NULL }
  ,{          "fuel_type", NULL }
  ,{           "interior", NULL }
  ,{           "exterior", NULL }
  ,{                "vin", NULL }
#endif
  ,{ NULL }
};
#endif

#if 0
//A lot of this stuff can be cross checked after the fact...
yamlList attrNodes[] = {
   {              "model", "table^thumb_table_a.tbody.tr.td^thumb_ymm.text" }
#if 1 
  ,{               "year", "`get_year`" }
  ,{               "make", "`get_make`" }
  ,{         "drivetrain", "" }
  ,{                "abs", "" }
  ,{   "air_conditioning", "" }
  ,{             "carfax", "" }
  ,{          "autocheck", "" }
  ,{           "autotype", "" }
  ,{               "fees", "" }
  ,{                "kbb", "" }

//These things MIGHT be able to be checked after the fact...
  ,{             "engine", "`get_engine`" } 
  ,{                "mpg", "`get_mpg`" }
  ,{          "fuel_type", "" }
  ,{           "interior", "" }
  ,{           "exterior", "" }
  ,{                "vin", "" }
#endif
  ,{ NULL }
};
#endif

//Test data
const char html_1[] = "<h1>Hello, World!</h1>";
const char html_file[] = "files/carri.html";
const char yamama[] = ""
"<html>"
	"<body>"
	"<div>"
		"<h2>Snazzy</h2>"
		"<p>The jakes is coming</p>"
		"<div>"
			"<p>The jakes are still chasing me</p>"
			"<div>"
				"<p>Son, why these dudes still chasing me?</p>"
				"<p>Bro, why these dudes still chasing me?</p>"
				"<p>Damn, why these dudes still chasing me?</p>"
			"</div>"
		"</div>"
		"<div>"
			"<p>The jakes are still chasing me</p>"
			"<div>"
				"<p>Son, why these dudes still chasing me?</p>"
				"<p>Bro, why these dudes still chasing me?</p>"
				"<p>Damn, why these dudes still chasing me?</p>"
			"</div>"
		"</div>"
	"</div>"
	"</body>"
"</html>"
;
