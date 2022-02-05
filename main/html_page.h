#ifndef HTMLPAGE_H
#define HTMLPAGE_H

static char* details_page = "<!doctype html><html><head><title>AirTalk - Setup Mode</title><style>body {	font-family: sans-serif;	padding: 32px;		background-color: #ffffff;	background-image:  repeating-linear-gradient(45deg, #000000 25%, transparent 25%, transparent 75%, #000000 75%, #000000), repeating-linear-gradient(45deg, #000000 25%, #ffffff 25%, #ffffff 75%, #000000 75%, #000000);	background-position: 0 0, 1px 1px;	background-size: 2px 2px;}form {	padding: none;	margin: none;	padding-left: 4em;	padding-bottom: 0.5em;}.d1 {	background: white;	border: 1px solid black;	padding: 2px;	width: 512px;}.d1 hr {	border: none;	border-top:	 3px double black;}.d2 {	border: 2px solid black;	padding: 0.5em;}.btns {	text-align: right;}</style></head></body><div class='d1'><div class='d2'>AirTalk Setup<hr><p>Connect to wireless network:</p><form method='GET'><table><tr><td>SSID:</td><td><input type='text' name='ssid'></td></tr>	<tr><td>Key:</td><td><input type='text' name='key'></td></tr><tr><td></td><td style='padding-top: 1em; padding-bottom: 1em;'><i>This will transmit the ssid and key in the query string and it will likely be readable in your browser history.  You may wish to avoid using this on shared computers.</i></td></tr></table><div class='btns'><input type='submit' value='Connect...'></div></form></div></div></body></html>";

#endif
