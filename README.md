Sioux is a framework to build Comet Web-Servers 
===============================================
 
It is based around a publish subscribe paradigm. A web client want to display a special set of data. If that data changes, the client want to get informed about that changes.

The project currently supports two protocols. The first is Bayeux, a protocol mainly used by the cometd project and pubsub/http an own protocol under development. The main difference between Bayeux and pubsub/http is, that Bayeux focuses on publishing messages, while pubsub/http is developed with communication object states in mind.

On the server side, a binding for C++ exists and a binding for Ruby/Rack (that makes it's easy to use it with Rails).

Dependencies
============

- Rake is used as build system 
- boost (1.50. will work)
- cometd client library (only for Bayeux protocol needed)
- jQuery 
- node.js (for Testing the CoffeeScript pubsub/http client)

getting "started"
=================

Currently this isn't an "out of the box" product. I've build the project under OS X and Linux. To get an overview, checkout the source and use `rake docu` to build doxygen documentation. `rake -T` gives an overview of the available build targets. If you need help, send me a message.

Examples
========

Have a look at /source/tests/chat.cpp as a C++ chat example and /source/tests/rack_chat.rb for the same in ruby.

Licence  
=======

Sioux is licensed under MIT Licence.


