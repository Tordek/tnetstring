

tnetstring:  data serialization using typed netstrings
======================================================


This is a data serialization library. It's a lot like JSON but it uses a
new syntax called "typed netstrings" that Zed has proposed for use in the
Mongrel2 webserver.  It's designed to be simpler and easier to implement
than JSON, with a happy consequence of also being faster.

An ordinary netstring is a blob of data prefixed with its length and postfixed
with a sanity-checking comma.  The string "hello world" encodes like this::

    11:hello world,

Typed netstings add other datatypes by replacing the comma with a type tag.
Here's the integer 12345 encoded as a tnetstring::

    5:12345#

And here's a list mixing integers and bools::

    19:5:12345#4:true!1:0#]

Also, since the ``,`` type tag represents a string, every valid netstring is a
valid typed netstring!

Simple enough?  This module gives you the following functions:

    :dumps:   dump an object as a tnetstring to a string
    :loads:   load a tnetstring-encoded object from a string
    :pop:     pop a tnetstring-encoded object from the front of a string


When I get around to it, I will also add the following:

    :dump:    dump an object as a tnetstring to a file
    :load:    load a tnetstring-encoded object from a file

Note that since parsing a tnetstring requires reading all the data into memory
at once, there's no efficiency gain from using the file-based versions of these
functions; I'm only planning to add them for API compatability with other
serialization modules e.g. pickle and json.

