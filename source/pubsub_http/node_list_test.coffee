sinon  = require 'sinon'
chai   = require 'chai'
chaiHelper = require './helpers_test'

require( './node_list.coffee' )

assert = chai.assert
`chai.use( chaiHelper );
expect = chai.expect`

describe "pubsub.http node_list", ->

    describe "starting with an empty list", ->
        
        beforeEach ->
            @list = new PubSub.NodeList
            
        it "will return no element, if empty", ->
            assert.strictEqual null, @list.get { a: '1', b: '2' }
            assert.strictEqual null, @list.get { abc: '1' }            
            assert.strictEqual null, @list.get { a: '1', b: '2', c: 'foo' }
            
        it "inserting an element will not cause an problems", ->
            @list.insert { a: '1', b: '2' }, 1
            @list.insert { abc: '1' }, { a: 1, b: true, c: false }
            
        it "will throw an exception if using keys, with no string values", ->
            for test in [ { a: 1 }, { abc: 'a', c:null }, { a: 'b', b: false }, { c: 1.4 } ] 
                assert.throws => 
                    @list.insert test, 1 
                , 'node name must contain only strings'                  
                
        it "will throw if key is not an object",->
            assert.throws =>
                @list.insert 1, 1
            , 'node name must be an object'
            
        it "will throw if node name is empty", ->
            assert.throws =>   
                @list.insert {}, 1
            , 'node name must not be empty'
            
        it "an inserted element will be found", ->
            @list.insert { a: 'a', b: '3' }, 42
            assert.strictEqual 42, @list.get { a: 'a', b: '3' }
            assert.strictEqual 42, @list.get { b: '3', a: 'a' }   
            
        it "will contain no elements", ->
            success = true
            
            @list.each ( name, node ) -> success = false
            
            assert success, 'empty list contains no element'
                        
    describe "starting with a not empty list", ->
                
        to_be_found = null          

        beforeEach ->
            @list = new PubSub.NodeList
            
            to_be_found = [
                { key: { v: "1", b: "a" }, value: 22 }  
                { key: { v: "2", b: "a" }, value: 23 }  
                { key: { v: "1", b: "c" }, value: 24 }  
                { key: { v: "1" }, value: 25 }  
                { key: { b: "a" }, value: 26 }  
            ]                             
            
            @list.insert( ele.key, ele.value ) for ele in to_be_found

        it "different elements can be stored in the list and found again", ->
            for ele in to_be_found
                 assert.strictEqual @list.get( ele.key ), ele.value
                 
        it "if one element removed, it will not be able to find all othere",->
            assert @list.remove( to_be_found[ 3 ].key )
            to_be_found.splice 3, 1
            
            for ele in to_be_found
                 assert.strictEqual @list.get( ele.key ), ele.value
                 
        it "if one element is removed, it should not be in the list", ->                 
            assert @list.remove( to_be_found[ 3 ].key )
            assert.strictEqual @list.get( to_be_found[ 3 ].key ), null

        it "should contain the new value, if updated", ->
            @list.insert to_be_found[ 0 ].key, 44
            to_be_found[ 0 ].value = 44
            
            for ele in to_be_found
                 assert.strictEqual @list.get( ele.key ), ele.value
                
        it "should iterate over all elements", ->
            elements = []
            
            @list.each ( name, node ) =>
                elements.push { key: name, value: node }
                
            expect( elements ).to.have.deepMembers( to_be_found )               

                                