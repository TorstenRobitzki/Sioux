assert = require("assert")

describe "pubsub.http interface", ->

    describe "when subscribing to a node", ->
    
        it "should perform a http request", ->
            assert.equal 1, 1