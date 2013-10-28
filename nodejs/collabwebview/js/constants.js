var GRUMPIES = {
	'short': {
		'text': "Grumpy wizards make toxic brew for the evil Queen and Jack. A quick movement of the enemy will jeopardize six gunboats. The job of waxing linoleum frequently peeves chintzy kids. My girl wove six dozen plaid jackets before she quit. Twelve ziggurats quickly jumped a finch box.",
		'columns': [ // single column
			{
				sizes: [72, 60, 48, 36, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12],
				innerblock: 'div'
			}
		],
		'tab': '#headlines'
	},

	'long': {
		'text': "Grumpy wizards make toxic brew for the evil Queen and Jack. One morning, when Gregor Samsa woke from troubled dreams, he found himself transformed in his bed into a horrible vermin. He lay on his armour-like back, and if he lifted his head a little he could see his brown belly, slightly domed and divided by arches into stiff sections. The bedding was hardly able to cover it and seemed ready to slide off any moment. His many legs, pitifully thin compared with the size of the rest of him, waved about helplessly as he looked. 01234567890 ",
		'columns': [ // two columns
			{sizes: [20, 19, 18, 17], innerblock: '.textsettingCol1'},
			{sizes: [16, 15, 14, 13, 12, 11, 10], innerblock: '.textsettingCol2'}
		],
		'tab': '#text'
	},

	'lowercaseShort': {
		'text': "the five boxing wizards jump quickly pack my red box with five dozen quality jugs a very big box sailed up then whizzed quickly from japan",
		'columns': [
			{sizes: [72, 60, 48, 36, 30, 24, 18], innerblock: 'div:first'}
		],
		'tab': '#lowercases'
	},

	'lowercaseLong': {
		'text': "serviced tightly trestle custom nosey impugned gooier deeper oat charade smashed welting clopping fondly discard welfare gaudy mission shoddily mooed knelling glance golfed trope togae knocked vulgarly gigabyte curbing snowball outback stepped marmot clayier coltish descry gratify root freedom puree urgency moist careered journal oracle felony marble salary readying besiege twitched ranching snoozing disk mister warbling outwit schism sudsiest street gondola blushing pennon smarted jiving sty rocker griping rocketry dieing tarring A very big box sailed up then whizzed quickly from Japan the five boxing wizards jump quickly pack my red box with five dozen quality jugs",
		'columns': [
			{sizes: [16, 15, 14], innerblock: '.textsettingCol1'},
			{sizes: [13, 12, 11, 10], innerblock: '.textsettingCol2'}
		],
		'tab': '#lowercases'
	},

	'adhesionShort': {
		'text': "adhesion donnishness indianians deaden on ode so sheenie died dashed dens seaside easines nonseasoned seen hindi said seines sadnesses deaden donnishness dissensions dead",
		'columns': [
			{sizes: [72, 60, 48, 36, 30, 24, 18], innerblock: 'div:first'}
		],
		'tab': '#adhesion'
	},

	'adhesionLong': {
		'text': "dined shoon hooded ahead shined hashed dinned soon nod nine sane inned ani session doe node idea side indeed aide anise hose noose noshed inside hid aeon inane none hoed nosed diseased handed noise haded sanded one hie deeded shine honed disdained hen ash said hah onion sodded donned and aha dine disease deed adenoid hoodooed deaned end denied anon iodine nosh shooed deadened sided addenda did aniseed sand hied nodded nose shoeshine hashish hissed done seed noon sensed sinned needed send nooned odd disdain shinned anion had shied",
		'columns': [
			{sizes: [16, 15, 14], innerblock: '.textsettingCol1'},
			{sizes: [13, 12, 11, 10], innerblock: '.textsettingCol2'}
		],
		'tab': '#adhesion'
	},

	'hamburgefonstivShort': {
		'text': "fortieth boring trait favoring barrage referring thrusting tannest embargo sausage gaining astutest augur hibernate variant hearse beggaring foresee eagerer hearten ensnaring tufting interstate meager veneration stigma feminine tabbing noising trimming throbbing ransoming stiffen oaring fishing rehire overbore bonniest ravishment teensiest mintier shriven unforeseen overrate surgeon smarter submarine revenging assuaging masher amnesia insentient rehashing fresher buttering sorghum thine sitter month serer minting variate torsion gaunt",
		'columns': [
			{sizes: [72, 60, 48, 36, 30, 24, 18], innerblock: 'div:first'}
		],
		'tab': '#hamburgefonstiv'
	},

	'hamburgefonstivLong': {
		'text': "fortieth boring trait favoring barrage referring thrusting tannest embargo sausage gaining astutest augur hibernate variant hearse beggaring foresee eagerer hearten ensnaring tufting interstate meager veneration stigma feminine tabbing noising trimming throbbing ransoming stiffen oaring fishing rehire overbore bonniest ravishment teensiest mintier shriven unforeseen overrate surgeon smarter submarine revenging assuaging masher amnesia insentient rehashing fresher buttering sorghum thine sitter month serer minting variate torsion gaunt berthing goofiest sober informing sourer tonight neigh iratest torte situate rheumier antagonist serening motion guise unseeing masseuse",
		'columns': [
			{sizes: [16, 15, 14], innerblock: '.textsettingCol1'},
			{sizes: [13, 12, 11, 10], innerblock: '.textsettingCol2'}
		],
		'tab': '#hamburgefonstiv'
	},

	'caps': {
		'text': "Arrowroot Barlety Chervil Dumpling Endine Flaxseed Garbanzo Hijiki Ishtu Jicama Kale Lycheen Marjoram Nectarine Oxtail Pizza Quinoa Roquefort Squash Tofu Uppuma Vanilla Wheat Xergis Yogurt Zweiback 0 1 2 3 4 5 6 7 8 9 ! ?",
		'columns': [
			{sizes: [60, 48, 36, 30, 28, 24, 20, 18, 16, 14, 13], innerblock: 'div'}
		],
		'tab': '#caps'
	},

	'allcaps': {
		'text': "ARROWROOT BARLETY CHERVIL DUMPLING ENDINE FLAXSEED GARBANZO HIJIKI ISHTU JICAMA KALE LYCHEEN MARJORAM NECTARINE OXTAIL PIZZA QUINOA ROQUEFORT SQUASH TOFU UPPUMA VANILLA WHEAT XERGIS YOGURT ZWEIBACK 0 1 2 3 4 5 6 7 8 9 ! ?",
		'columns': [
			{sizes: [60, 48, 36, 30, 28, 24, 20, 18, 16, 14, 13], innerblock: 'div'}
		],
		'tab': '#allcaps'
	}
};

var hintsCaps = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
var hintsLower = "abcdefghijklmnopqrstuvwxyz.:;,";
var hintsNumbers= "1234567890 @ &amp;!?#$€%";


function eventTextLineChange(e) {
	var $this = $(this);
	bulkChangeTextForTab(e.data.grumpy, $this.text(), $this);
}


function onblur() {
	var $this = $(this);
	var text = $this.html();
	if ($this.data('enter') !== text) {
		$this.data('enter', text);
		$this.trigger({type: 'change', action : 'save'});
	}
	return $this;
}


function onkeyup() {
	var $this = $(this);
	var text = $this.html();
	if ($this.data('before') !== text) {
		$this.data('before', text);
		$this.trigger({type: 'change', action : 'update'});
	}
	return $this;
}


function onfocus() {
	var $this = $(this);
	$this.data('enter', $this.html());
	$this.data('before', $this.html());
	return $this;
}


function setColumnTemplate(container, grumpy) {
	for (var k = 0; k < grumpy.columns.length; k++) {

		var sizes = grumpy.columns[k].sizes;
		var block = container.find(grumpy.columns[k].innerblock);
		for (var i = 0; i < sizes.length; i++) {
			fontsize = sizes[i].toString();
			block.append($('<p>').addClass('sizelabel').text(fontsize + 'px'));

			var textline = $('<p>').addClass('textline')
				.css('font-size', fontsize + 'px')
				.attr('contenteditable', true)
				.text(grumpy.text)
				.on('focus', onfocus)
				.on('keyup paste', onkeyup)
				.on('blur', onblur)
				.on('change', {'grumpy': grumpy}, eventTextLineChange);

			block.append(textline);
			//block.append($('<p>&nbsp;</p>'));
		}
	}
}

function setColumnTemplate2(container, grumpy) {
	for (var k = 0; k < grumpy.columns.length; k++) {

		var sizes = grumpy.columns[k].sizes;
		var block = container.find(grumpy.columns[k].innerblock);
		for (var i = 0; i < sizes.length; i++) {
			fontsize = sizes[i].toString();
			block.append($('<p>').addClass('sizelabel').text(fontsize + 'px'));

			var textline = $('<p>').addClass('textline')
				.css('font-size', fontsize + 'px')
				.attr('contenteditable', true)
				.text(grumpy.text)
				.on('focus', onfocus)
				.on('keyup paste', onkeyup)
				.on('blur', onblur)
				.on('change', {'grumpy': grumpy}, eventTextLineChange);

			block.append(textline);
			block.append($('<p>&nbsp;</p>'));
		}
	}
}


function setSplitSingleToDual(container, grumpies_short, grumpies_long) {
	setColumnTemplate(container, grumpies_short);
	setColumnTemplate(container, grumpies_long);
}


function bulkChangeTextForTab(grumpy, value, except) {
	for (var i = 0; i < grumpy.columns.length; i++) {
		$(grumpy.tab).find(grumpy.columns[i].innerblock).find('.textline').not(except).text(value);
	}
}


function prepareAndShowFontLayout() {

	setColumnTemplate($('#headlines'), GRUMPIES.short);

	setColumnTemplate2($('#text'), GRUMPIES.long);

	var $lowercases = $('#lowercases');
	setColumnTemplate($lowercases, GRUMPIES.lowercaseShort);
	setColumnTemplate2($lowercases, GRUMPIES.lowercaseLong);

	var $adhesion = $('#adhesion');
	setColumnTemplate($adhesion, GRUMPIES.adhesionShort);
	setColumnTemplate2($adhesion, GRUMPIES.adhesionLong);

	var $hamburgefonstiv = $('#hamburgefonstiv');
	setColumnTemplate($hamburgefonstiv, GRUMPIES.hamburgefonstivShort);
	setColumnTemplate2($hamburgefonstiv, GRUMPIES.hamburgefonstivLong);

	var $caps = $('#caps');
	setColumnTemplate2($caps, GRUMPIES.caps);
	
	var $allcaps = $('#allcaps');
	setColumnTemplate2($allcaps, GRUMPIES.allcaps);

	var target = document.createElement('div');
	target.setAttribute('style','width: 920px;');
	target.setAttribute('contenteditable','true');
	for (var a = 28; a>8; a--) { 
		var sizelabel = document.createElement('p');
		sizelabel.setAttribute('class','sizelabel');
		sizelabel.textContent = a + 'px';
		target.appendChild(sizelabel);

		var hintslower = document.createElement('p'); 
		hintslower.setAttribute('class','hints-lower');
		hintslower.setAttribute('style','font-size: ' + a + 'px');
		target.appendChild(hintslower);

		var hintscaps = document.createElement('p'); 
		hintscaps.setAttribute('class','hints-caps');
		hintscaps.setAttribute('style','font-size: ' + a + 'px');
		target.appendChild(hintscaps);

		var hintsnumbers = document.createElement('p'); 
		hintsnumbers.setAttribute('class','hints-numbers');
		hintsnumbers.setAttribute('style','font-size: ' + a + 'px');
		target.appendChild(hintsnumbers);

		var hintsend = document.createElement('p'); 
		hintsend.innerHTML = '&nbsp;';
		target.appendChild(hintsend);
	};
	document.getElementById('hinting').appendChild(target);

	var hints_caps = $('.hints-caps');
	var hints_numbers = $('.hints-numbers');
	var hints_lower = $('.hints-lower');
	hints_lower.html(hints_lower.html() + hintsLower);
	hints_caps.html(hints_caps.html() + hintsCaps);
	hints_numbers.html(hints_numbers.html() + hintsNumbers);
}