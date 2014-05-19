/*
 * font dragr v1.5
 * http://www.thecssninja.com/javascript/font-dragr
 * Copyright (c) 2010 Ryan Seddon
 * released under the MIT License
 */
var TCNDDF = TCNDDF || {};

(function () {
	var dropContainer,
		dropListing,
		displayContainer,
		domElements,
		fontPreviewFragment = document.createDocumentFragment(),
		styleSheet,
		fontFaceStyle,
		persistantStorage = window.localStorage || false,
		webappCache = window.applicationCache || window,
		contentStorageTimer;

	TCNDDF.setup = function () {
		dropListing = document.getElementById("versions");
		dropContainer = document.getElementById("dropcontainer");
		displayContainer = document.getElementById("custom");
		styleSheet = document.styleSheets[0];

		dropListing.addEventListener("click", TCNDDF.handleFontChange, false);

		/* LocalStorage events */
		if (persistantStorage) {
			displayContainer.addEventListener("keyup", function () {
				contentStorageTimer = setTimeout("TCNDDF.writeContentEdits('mainContent')", 1000);
			}, false);
			displayContainer.addEventListener("keydown", function () {
				clearTimeout(contentStorageTimer);
			}, false);

			// Restore changed data
			TCNDDF.readContentEdits("mainContent");
		}

		/* DnD event listeners */
		dropContainer.addEventListener("dragenter", function (event) {
			TCNDDF.preventActions(event);
		}, false);
		dropContainer.addEventListener("dragover", function (event) {
			TCNDDF.preventActions(event);
		}, false);
		dropContainer.addEventListener("drop", TCNDDF.handleDrop, false);

		/* Offline event listeners */
		webappCache.addEventListener("updateready", TCNDDF.updateCache, false);
		webappCache.addEventListener("error", TCNDDF.errorCache, false);
	};

	TCNDDF.handleDrop = function (evt) {
		var dt = evt.dataTransfer,
			files = dt.files || false,
			count = files.length,
			acceptedFileExtensions = /^.*\.(ttf|otf|woff)$/i;


		TCNDDF.preventActions(evt);

		for (var i = 0; i < count; i++) {
			var file = files[i],
				droppedFullFileName = file.name,
				droppedFileName,
				droppedFileSize;

			if (droppedFullFileName.match(acceptedFileExtensions)) {
				droppedFileName = droppedFullFileName.replace(/\..+$/, ""); // Removes file extension from name
				droppedFileName = droppedFileName.replace(/\W+/g, "-"); // Replace any non alpha numeric characters with -
				droppedFileSize = Math.round(file.size / 1024) + "kb";

				TCNDDF.processData(file, droppedFileName, droppedFileSize);
			} else {
				alert("Invalid file extension. Will only accept ttf, otf or woff font files");
			}
		}
	};

	TCNDDF.processData = function (file, name, size) {
		var reader = new FileReader();
		reader.name = name;
		reader.size = size;
	/* 
		Chrome 6 dev can't do DOM2 event based listeners on the FileReader object so fallback to DOM0
		http://code.google.com/p/chromium/issues/detail?id=48367
		reader.addEventListener("loadend", TCNDDF.buildFontListItem, false);
	*/
		reader.onloadend = function (event) {
			TCNDDF.buildFontListItem(event);
		}
		reader.readAsDataURL(file);
	};

	TCNDDF.buildFontListItem = function (event) {
		domElements = [
				document.createElement('li'),
				document.createElement('span'),
				document.createElement('span')
		];

		var name = event.target.name,
			size = event.target.size,
			data = event.target.result;

		// Dodgy fork because Chrome 6 dev doesn't add media type to base64 string when a dropped file(s) type isn't known
		// http://code.google.com/p/chromium/issues/detail?id=48368
		var dataURL = data.split("base64");

		if (dataURL[0].indexOf("application/octet-stream") == -1) {
			dataURL[0] = "data:application/octet-stream;base64"

			data = dataURL[0] + dataURL[1];
		}

		// Get font file and prepend it to stylsheet using @font-face rule
		fontFaceStyle = "@font-face{font-family: " + name + "; src:url(" + data + ");}";
		styleSheet.insertRule(fontFaceStyle, 0);

		domElements[2].appendChild(document.createTextNode(size));
		domElements[1].appendChild(document.createTextNode(name));
		domElements[0].className = "active";
		domElements[0].title = name;
		domElements[0].style.fontFamily = name;
		domElements[0].appendChild(domElements[1]);
		domElements[0].appendChild(domElements[2]);

		fontPreviewFragment.appendChild(domElements[0]);

		dropListing.insertBefore(fontPreviewFragment, dropListing.firstChild);
		// Comment the preceding line and uncomment the following to append FontListItems instead of prepending them
		// dropListing.appendChild(fontPreviewFragment);
		TCNDDF.updateActiveFont(domElements[0]);
		displayContainer.style.fontFamily = name;
	};

	/* Control changing of fonts in drop list  */
	TCNDDF.handleFontChange = function (evt) {
		var clickTarget = evt.target || window.event.srcElement;

		if (clickTarget.parentNode === dropListing) {
			TCNDDF.updateActiveFont(clickTarget);
		} else {
			clickTarget = clickTarget.parentNode;
			TCNDDF.updateActiveFont(clickTarget);
		}
	};
	TCNDDF.updateActiveFont = function (target) {
		var getFontFamily = target.title,
			dropListItem = dropListing.getElementsByTagName("li");

		displayContainer.style.fontFamily = getFontFamily;

		for (var i = 0, len = dropListItem.length; i < len; i++) {
			dropListItem[i].className = "";
		}
		target.className = "active";
	};

	/* localStorage methods */
	TCNDDF.readContentEdits = function (storageKey) {
		var editedContent = persistantStorage.getItem(storageKey);

		if ( !! editedContent && editedContent !== "undefined") {
			displayContainer.innerHTML = editedContent;
		}
	};
	TCNDDF.writeContentEdits = function (storageKey) {
		var content = displayContainer.innerHTML;

		persistantStorage.setItem(storageKey, content);
	};

	/* Offline cache methods */
	TCNDDF.updateCache = function () {
		webappCache.swapCache();
		console.log("Cache has been updated due to a change found in the manifest");
	};
	TCNDDF.errorCache = function () {
		console.log("You're either offline or something has gone horribly wrong.");
	};

	/* Util functions */
	TCNDDF.preventActions = function (evt) {
		if (evt.stopPropagation && evt.preventDefault) {
			evt.stopPropagation();
			evt.preventDefault();
		} else {
			evt.cancelBubble = true;
			evt.returnValue = false;
		}
	};


	/* 2013-04-21 Added buildFontListItemFromURL for collab */
	TCNDDF.buildFontListItemFromCollabJsonData = function (data) {

		var name = 'font-' + data.seq,
			url = "http://" + location.host + '/' + data.earl,
			size = 'XXXkb', // TODO add this to JSON?
			// 2013-04-19 DC Not sure why I have to duplicate these vars for Chrome, FFox doesn't need them, but duping them here works
			dropListing = document.getElementById("versions"),
			dropContainer = document.getElementById("dropcontainer"),
			displayContainer = document.getElementById("custom"),
			styleSheet = document.styleSheets[0];

		// Get font file and prepend it to stylsheet using @font-face rule
		fontFaceStyle = "@font-face{font-family: '" + name + "'; src:url('" + url + "');}";
		styleSheet.insertRule(fontFaceStyle, 0);

		// DEBUG console.log(data.codepoint)
		if ( 0 > data.codepoint ) {
			latestChar = 'unencoded glyph';
			canHighlightChange = 0;
		} else {
			latestChar = String.fromCharCode(data.codepoint);
			canHighlightChange = 1;
		};
				
/*
		// TODO 
		if ( canHighlightChange = 1 ) {
			// TODO highlight
		}
*/
		domElements = [
			version,
			document.createElement('ul'),
			document.createElement('li'),
			document.createElement('li'),
			document.createElement('li'),
			document.createElement('li'),
		];


		version = document.createElement('li'),
		version.className = "active";
		version.title = name;
		version.appendChild(document.createTextNode(name));
		
		domElements[5].appendChild(document.createTextNode(data.glyph));
		domElements[4].appendChild(document.createTextNode(data.codepoint+ ' (' + latestChar + ')'));
		link = document.createElement('a');
		link.href = url
		domElements[3].appendChild(document.createTextNode(url));
		domElements[2].appendChild(document.createTextNode(size));
		version.appendChild(document.createTextNode(name));
		domElements[4].style.fontFamily = name;

		version.appendChild(domElements[1]);
		domElements[1].appendChild(domElements[2]);
		domElements[1].appendChild(link);
		link.appendChild(domElements[3]);
		domElements[1].appendChild(domElements[4]);
		domElements[1].appendChild(domElements[5]);

		fontPreviewFragment.appendChild(version);

		dropListing.insertBefore(fontPreviewFragment, dropListing.firstChild);
		// Comment the preceding line and uncomment the following to append FontListItems instead of prepending them
		// dropListing.appendChild(fontPreviewFragment);
		TCNDDF.updateActiveFont(domElements[0]);
		displayContainer.style.fontFamily = name;
		// DEBUG console.dir(fontFaceStyle)
	};

	window.addEventListener("load", TCNDDF.setup, false);
})();
