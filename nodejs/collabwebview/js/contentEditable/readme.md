# jQuery contentEditable

Automatically adds events on 'contenteditable' fields and returns the changed data for further processing (and saving)

## Usage

In its basic implementation, the plugin method contentEditable() is attached to the parent container that has all the 'contenteditable' fields. 

```
$("#...").contentEditable().change( myFunction );
```

A chained method can be attached to monitor the "change" trigger so the updates can be captured and saved.

Note that this only needs to be executed once for all included fields of the container. 


## Response

The object that's returned on a "change" event is a standard jQuery Event() object with the addition of these variables: 

* **action** : possible values are _update_ & _save_ , depending on the focus state of the field.
* **changed** : containing the value of the changed field. the key is defined from a data-key attribute passed in the markup. 

## Options

Currently there are no parameters for this plugin

## Credits

Created by [Makis Tracend](http://tracend.me)

Originally realeased as a [gist](https://gist.github.com/tracend/3410122) to address [this issue](http://stackoverflow.com/a/6263537).

Distributed by [Makesites.org](http://makesites.org)

Released under the [MIT license](http://makesites.org/licenses/MIT)
