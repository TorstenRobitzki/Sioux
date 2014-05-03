assert = require( 'chai' ).assert

require './update.coffee'

update_at       = 1
delete_at       = 2
insert_at       = 3
delete_range    = 4
update_range    = 5
edit_at         = 6

describe "PubSub.update", ->

    describe 'given the update operation is empty', ->

        update = ( input )->
            PubSub.update input, []

        it "simply returns the input value", ->
            assert.deepEqual update( [1,2,3] ), [1,2,3]
            assert.deepEqual update( [] ), []
            assert.deepEqual update( {} ), {}
            assert.deepEqual update( {a:1, b:2} ), {a:1, b:2}
            assert.deepEqual update( 1 ), 1
            assert.deepEqual update( "asdasdd" ), "asdasdd"

    describe "given the input is an empty array", ->
        
        update = ( update_instructions )->
            PubSub.update( [], update_instructions )

        describe "and the update operation is an 'update_at'", ->
            it "raises an exception, if the index is negative", ->
                test = -> update [ update_at, -4, 1 ] 

                assert.throw test, RangeError, 'bad index for update: -4'

            it "raises an exception, if the index is greater or equal to the input size", ->
                test = -> update [ update_at, 0, 0 ]

                assert.throw test, RangeError, 'bad index for update: 0'

        describe "and the update operation is an 'delete_at'", ->
            it "raises an exception, if the index is negative", ->
                test = -> update [ delete_at, -4 ] 

                assert.throw test, RangeError, 'bad index for delete: -4'

            it "raises an exception, if the index is greater or equal to the input size", ->
                test = -> update [ delete_at, 0 ]

                assert.throw test, RangeError, 'bad index for delete: 0'

        describe "and the update operation is an 'insert_at'", ->

            it "raises an exception, if the index is negative", ->
                test = -> update [ insert_at, -4, 1 ] 

                assert.throw test, RangeError, 'bad index for insert: -4'

            it 'inserts an element, if the index is 0', ->
                assert.deepEqual update( [insert_at, 0, 17] ), [17]

            it "raises an exception, if the index is greater than the input size", ->
                test = -> update [ insert_at, 1, 1 ]

                assert.throw test, RangeError, 'bad index for insert: 1'

        describe "and the update operation is an 'delete_range'", ->

            it "raises an exception", ->
                test = -> update [ delete_range, 1, 4 ]

                assert.throw test, RangeError, 'bad range for delete: [1,4]'

        describe "and the update operation is an 'update_range'", ->

            it "raises an exception, if the range is not empty beginning with 0", ->
                test = ( s, e ) ->
                    -> update [ update_range, s, e, [ 1, 2, 3 ] ]

                assert.throw test( 1, 1 ), RangeError, 'bad range for update: [1,1]'
                assert.throw test( 1, 2 ), RangeError, 'bad range for update: [1,2]'

            it "replaces the empty array when the given range is empty and starting with 0", ->
                assert.deepEqual update( [ update_range, 0, 0, [ 1, 2 ] ] ), [ 1, 2 ]

        describe "and the update operation is an 'edit_at'", ->
            it "raises an exception", ->
                test = -> update [ edit_at, 0, [] ]

                assert.throw test, RangeError, 'bad index for edit: 0'

        describe "and the update operation is a mix out of simple operations", ->
            it "will apply all operations in the given order", ->
                assert.deepEqual update( [ 3, 0, 3, 3, 0, 3, 1, 1, 'foo', 2, 0 ]), [ 'foo' ]

    describe "given the input is a not empty array", ->
        update = ( update_instructions )->
            PubSub.update( [ 1, 2, 3, 4 ], update_instructions )

        describe "and the update operation is an 'update_at'", ->
            it "raises an exception, if the index is negative", ->
                test = -> update [ update_at, -1, 1 ] 

                assert.throw test, RangeError, 'bad index for update: -1'

            it "raises an exception, if the index is greater or equal to the input size", ->
                test = -> update [ update_at, 4, 1 ]

                assert.throw test, RangeError, 'bad index for update: 4'

            it "updates the first element", ->
                assert.deepEqual update( [ update_at, 0, 'test' ] ), [ 'test', 2, 3, 4 ]

            it "updates the last element", ->
                assert.deepEqual update( [ update_at, 3, 'test' ] ), [ 1, 2, 3, 'test' ]

            it "updates an element in the middle", ->
                assert.deepEqual update( [ update_at, 1, 'test' ] ), [ 1, 'test', 3, 4 ]

        describe "and the update operation is an 'delete_at'", ->        
            it "raises an exception, if the index is negative", ->
                test = -> update [ delete_at, -14 ] 

                assert.throw test, RangeError, 'bad index for delete: -14'

            it "raises an exception, if the index is greater or equal to the input size", ->
                test = -> update [ delete_at, 4 ]

                assert.throw test, RangeError, 'bad index for delete: 4'

            it 'deletes an element at the beginning', ->
                assert.deepEqual update( [ delete_at, 0 ] ), [ 2, 3, 4 ]

            it 'deletes an element in the middle', ->
                assert.deepEqual update( [ delete_at, 1 ] ), [ 1, 3, 4 ]

            it 'deletes an element at the end', ->
                assert.deepEqual update( [ delete_at, 3 ] ), [ 1, 2, 3 ]

        describe "and the update operation is an 'insert_at'", ->

            it "raises an exception, if the index is negative", ->
                test = -> update [ insert_at, -4, 1 ] 

                assert.throw test, RangeError, 'bad index for insert: -4'

            it 'inserts an element at the beginning', ->
                assert.deepEqual update( [insert_at, 0, 0] ), [ 0, 1, 2, 3, 4 ]

            it 'inserts an element at the end', ->
                assert.deepEqual update( [insert_at, 4, 5] ), [ 1, 2, 3, 4, 5 ]

            it 'inserts an element in the middle', ->
                assert.deepEqual update( [insert_at, 2, 17] ), [ 1, 2, 17, 3, 4 ]

            it "raises an exception, if the index is greater than the input size", ->
                test = -> update [ insert_at, 5, 14 ]

                assert.throw test, RangeError, 'bad index for insert: 5'

        describe "and the update operation is an 'delete_range'", ->
            it "raises an exception, if the range is not within the array", ->
                test = ( s, e )->
                    ()-> update [ delete_range, s, e ]

                assert.throw test( 0, 5 ), RangeError, 'bad range for delete: [0,5]'
                assert.throw test( -1, 1 ), RangeError, 'bad range for delete: [-1,1]'
                assert.throw test( 1, 7 ), RangeError, 'bad range for delete: [1,7]'

            it "raises an exception, if the end of range is not behind the start", ->
                test = -> update [ delete_range, 1, 1 ]

                assert.throw test, RangeError, 'bad range for delete: [1,1]'

            it "deletes the given range", ->
                assert.deepEqual update( [ delete_range, 0, 4 ] ), []
                assert.deepEqual update( [ delete_range, 0, 1 ] ), [ 2, 3, 4 ]
                assert.deepEqual update( [ delete_range, 3, 4 ] ), [ 1, 2, 3 ]
                assert.deepEqual update( [ delete_range, 0, 3 ] ), [ 4 ]
                assert.deepEqual update( [ delete_range, 1, 4 ] ), [ 1 ]

        describe "and the update operation is an 'update_range'", ->
            it "raises an exception, if the given range is invalid", ->
                test = ( s, e )->
                    ()-> update [ update_range, s, e ]

                assert.throw test( 0, 5 ), RangeError, 'bad range for update: [0,5]'
                assert.throw test( -1, 1 ), RangeError, 'bad range for update: [-1,1]'
                assert.throw test( 1, 7 ), RangeError, 'bad range for update: [1,7]'

            it "replaces the given range", ->
                assert.deepEqual update( [ update_range, 0, 4, [] ] ), []
                assert.deepEqual update( [ update_range, 1, 3, [ 'a', 'b' ] ] ), [ 1, 'a', 'b', 4 ]


            it "acts like an insert, if the range is empty", ->
                assert.deepEqual update( [ update_range, 0, 0, [ -1, 0 ] ] ), [ -1, 0, 1, 2, 3, 4 ]
                assert.deepEqual update( [ update_range, 4, 4, [ 'a', 'b' ] ] ), [ 1, 2, 3, 4, 'a', 'b' ]

        describe "and the update operation is an 'edit_at'", ->

            it "updates an array", ->
                assert.deepEqual(
                    PubSub.update( [ 1, 2, [ 1, 2 ], 4 ], [ edit_at, 2, [ delete_at, 0 ] ] ),
                    [ 1, 2, [ 2 ], 4 ] )

            it "updates an object", ->
                assert.deepEqual(
                    PubSub.update( [ 1, 2, {}, 4 ], [ edit_at, 2, [ insert_at, 'a', 42 ] ] ),
                    [ 1, 2, { a: 42 }, 4 ] )

    describe "given the input is an empty object", ->
        update = ( update_operations )->
            PubSub.update( {}, update_operations )

        describe "and the update operation is an 'update_at'", ->
            it "raises an exception", ->
                test = -> update [ update_at, 'foo', 'bar' ]

                assert.throw test, RangeError, 'bad index for update: foo'

        describe "and the update operation is an 'delete_at'", ->
            it "raises an exception", ->
                test = -> update [ delete_at, 'foo', 'bar' ]

                assert.throw test, RangeError, 'bad index for delete: foo'

        describe "and the update operation is an 'insert_at'", ->
            it "inserts that element", ->
                assert.deepEqual update( [ insert_at, 'a', 42 ] ), { a: 42 }

        describe "and the update operation is an 'delete_range'", ->
            it "will throw",->
                assert.throw -> update( [ delete_range, 0, 1, [] ] )

        describe "and the update operation is an 'update_range'", ->
            it "will throw",->
                assert.throw -> update( [ update_range, 0, 1, [] ] )

        describe "and the update operation is an 'edit_at'", ->
            it "will throw",->
                assert.throw -> update( [ edit_at, 'a',[ insert_at, 0, 1 ] ] )

    describe "given the input is a not empty object", ->
        update = ( update_operations )->
            PubSub.update { a: 1, b: 2 }, update_operations

        describe "and the update operation is an 'update_at'", ->
            it "raises an exception, if the key is not valid", ->
                test = -> update [ update_at, 'foo', 'bar' ]

                assert.throw test, RangeError, 'bad index for update: foo'

            it "updates the given object", ->
                assert.deepEqual update( [ update_at, 'b', 'changed' ] ), { a: 1, b: 'changed' }

        describe "and the update operation is an 'delete_at'", ->
            it "raises an exception, if that key is not valid", ->
                test = -> update [ delete_at, 'foo', 'bar' ]

                assert.throw test, RangeError, 'bad index for delete: foo'

            it "deletes the given key", ->
                assert.deepEqual update( [ delete_at, 'b' ] ), { a: 1 }

        describe "and the update operation is an 'insert_at'", ->
            it "raises an exception, if that key is already given", ->
                test = -> update [ insert_at, 'a', 'bar' ]

                assert.throw test, RangeError, 'bad index for insert: a'

            it "inserts the given element", ->
                assert.deepEqual update( [ insert_at, 'c', 3 ] ), { a: 1, b: 2, c: 3 }

        describe "and the update operation is an 'delete_range'", ->
            it "will throw",->
                assert.throw -> update( [ delete_range, 0, 1, [] ] )

        describe "and the update operation is an 'update_range'", ->
            it "will throw",->
                assert.throw -> update( [ update_range, 0, 1, [] ] )

        describe "and the update operation is an 'edit_at'", ->
            it "will throw, if the given key doesn't exist", ->
                test = -> update [ edit_at, 'c', [ insert_at, 0 , 1 ] ]

                assert.throw test, RangeError, 'bad index for edit: c'

            it "updates an array", ->
                assert.deepEqual(
                    PubSub.update( { a: 1, b: [ 1, 2 ] }, [ edit_at, 'b', [ delete_at, 0 ] ] ),
                     { a: 1, b: [ 2 ] } )

            it "updates an object", ->
                assert.deepEqual(
                    PubSub.update( { a: 1, b: {} }, [ edit_at, 'b', [ insert_at, 'a', 42 ] ] ),
                    { a: 1, b: { a: 42 } } )

    describe "given the input and update is a little bit more complex", ->
        input  = -> [ 1, 2, { a: 'Hallo' } ]
        update = -> [
            insert_at, 0, 4,                # -> [ 4, 1, 2, { a: 'Hallo' } ]
            update_at, 3, [ 'a', 'Hallo'],  # -> [ 4, 1, 2, [ 'a', 'Hallo' ] ]
            delete_at, 1,                   # -> [ 4, 2, [ 'a', 'Hallo' ] ]
            update_range, 2, 2, [ 1, 2 ],   # -> [ 4, 2, 1, 2, [ 'a', 'Hallo' ] ]
            delete_range, 0, 2,             # -> [ 1, 2, [ 'a', 'Hallo' ] ]
            edit_at, 2, [ delete_at, 1 ]    # -> [ 1, 2, [ 'a' ] ]
            ]

        it "will change the argument", ->
            input_copy = input()
            result = PubSub.update input_copy, update

            assert.deepEqual input_copy, result

        it "will apply all operations", ->
            assert.deepEqual PubSub.update( input(), update() ), [ 1, 2, [ 'a' ] ]



