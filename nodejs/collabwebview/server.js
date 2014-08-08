String.prototype.endsWith = function(suffix) {
    return this.indexOf(suffix, this.length - suffix.length) !== -1;
};
if (typeof String.prototype.startsWith != 'function') {
  // see below for better implementation!
  String.prototype.startsWith = function (str){
    return this.indexOf(str) == 0;
  };
}

var app = require('http').createServer(handler)
  , io = require('socket.io').listen(app)
  , fs = require('fs');

// creating the server ( localhost:8000 ) 
app.listen( 8000 );

// on server started we can load our client.html page
function handler ( req, res ) {
     console.dir("x req " + req.url );

if( req.url.startsWith("/js/") || req.url.startsWith("/css/")) {
  fs.readFile( __dirname + req.url ,
  function ( err, data ) {
    if ( err ) {
      console.log( err );
      res.writeHead(500);
      return res.end( 'Error loading font!' );
    }
    res.writeHead( 200 );
console.dir( data );
    res.end( data );
  });
  return;
} 

if( req.url.endsWith(".ttf")) {
  console.log("ttf request: " + req.url );
  fs.readFile( '/tmp/fontforge-collab/ffc' + req.url ,
  function ( err, data ) {
    if ( err ) {
      console.log( err );
      res.writeHead(500);
      return res.end( 'Error loading font!' );
    }

    res.setHeader('Cache-Control', 'private, no-cache, no-store, must-revalidate');
    res.setHeader('Expires', '-1');
    res.setHeader('Pragma', 'no-cache');
    res.end( data );
  });

} else {


  fs.readFile( __dirname + '/index.html' ,
  function ( err, data ) {
    if ( err ) {
      console.log( err );
      res.writeHead(500);
      return res.end( 'Error loading index.html' );
    }
    res.writeHead( 200 );
    res.end( data );
  });
}
};

// creating a new websocket to keep the content updated without any AJAX request
io.sockets.on( 'connection', function ( socket ) {

  // watching the collab files
  fs.watch( '/tmp/fontforge-collab/ffc', function ( event, fn ) {
     console.log("fn " + fn );

     if( fn.endsWith(".json")) {


    // on file change we can read the new xml
    var options = new Object();
    options.encoding = "UTF-8";
    fs.readFile( "/tmp/fontforge-collab/ffc/" + fn, function ( err, data ) {
      if ( err ) throw err;

    var result = new Object();
console.log(data);
var t = JSON.parse(data.toString());
t.earl = fn.replace(".json",".ttf");
    result.json = JSON.stringify( t );
    //result.json = data.toString();
    result.time = new Date();
    result.sfnt = fn.replace(".json",".ttf")
    socket.volatile.emit( 'notification' , result );
    });
    }
  });
});

