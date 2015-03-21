#!/usr/bin/env node

var express = require('express');
var bodyParser = require('body-parser');
var fs = require('fs-extra');
var multer = require('multer');
var app = express();
var child_process = require('child_process');

app.use(bodyParser.urlencoded({ extended: false }));
app.use(multer({ dest: './uploads/'
	       , limits: {
		   files: 1,
		   fields: 15,
		   fileSize: 1024 * 1024
	       }
}))

//app.use(express.bodyParser());
//app.use(bodyParser.urlencoded({ extended: false }));
//app.use(express.bodyParser({defer: true}));

app.get('/', function(req, res){
    console.log('GET /')
    var html = "nothing to see here";
    res.writeHead(200, {'Content-Type': 'text/html'});
    res.end(html);
});

app.post('/breakpad', function(req, res){
    console.log('POST to breakpad');
    console.dir(req.body);
    console.log(req.files);

    console.log(req.files["upload_file_minidump"]);
    if( req.files["upload_file_minidump"] === undefined 
	|| req.files["upload_file_minidump"]["name"] === undefined ) 
    {
	res.writeHead(200, {'Content-Type': 'text/html'});
	res.end('sorry, you have bad data!');
	return;
    }

    var dumpFilePath = req.files["upload_file_minidump"]["path"];
    console.log("filename: " + dumpFilePath);

    var githash = req.body["FontForgeGitHash"];
    var md = {
	ver:         req.body["ver"]
	, githash:   req.body["FontForgeGitHash"]
	, buildtime: req.body["FontForgeBuildTime"]
	, ptime:     req.body["ptime"]
	, guid:      req.body["guid"]
        , useragent: req.headers['user-agent']
    };
    fs.outputJsonSync( dumpFilePath + ".json", md );

    res.writeHead(200, {'Content-Type': 'text/html'});
    res.end('thanks');

    
    var basename = dumpFilePath;
    basename.replace(/\.[^/.]+$/, "");

    var symbolsDir = "./breakpad-symbols-by-hash/" + githash + "/breakpad/symbols/x86_64";
    console.log("dumpFilePath:" + dumpFilePath );
    console.log("symbolsDir  :" + symbolsDir );
    var child = child_process.execFile("./minidump_stackwalk", 
				       [ dumpFilePath, symbolsDir ],
          function (error, stdout, stderr) {

	      fs.outputFile( dumpFilePath + ".backtrace", stdout, function(err) {
		  console.log(err) // => null
	      })
              // console.log('stdout: ' + stdout);
              // console.log('stderr: ' + stderr);
              if (error !== null) {
                  console.log('exec error: ' + error);
              }
          });

});

port = 3000;
app.listen(port);
console.log('Listening at http://localhost:' + port)
