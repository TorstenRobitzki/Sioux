module.exports = function(chai, utils) {
    var Assertion = chai.Assertion;
    
    function contains(elem, superset) {
        return !superset.every(function(selem){
            return !utils.eql(elem,selem);
        });
    }
    
    function isSubsetOf(subset, superset) {
        return subset.every(function(elem) {
            return contains(elem, superset);
        });
    }
    
    Assertion.addMethod( 'deepMembers', function (subset, msg) {
        if (msg) utils.flag(this, 'message', msg);
        var obj = utils.flag(this, 'object');

        new Assertion(obj).to.be.an('array');
        new Assertion(subset).to.be.an('array');

        if (utils.flag(this, 'contains')) {
          return this.assert(
              isSubsetOf(subset, obj)
            , 'expected #{this} to be a deep superset of #{act}'
            , 'expected #{this} to not be a deep superset of #{act}'
            , obj
            , subset
          );
        }
    
        this.assert(
            isSubsetOf(obj, subset) && isSubsetOf(subset, obj)
            , 'expected #{this} to have the same deep members as #{act}'
            , 'expected #{this} to not have the same deep members as #{act}'
            , obj
            , subset
        );
    });    
}
  