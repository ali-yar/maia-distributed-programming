angular.module("itncApp.services")

.factory("Templates", ["$firebaseArray", function($firebaseArray) {
  var templatesRef = firebase.database().ref().child("mammograms");
  templates = $firebaseArray(templatesRef);

  function getAll(uid) {
    return templates.$loaded().then(function(list) {
      console.log(list)
      var arr = [];
      angular.forEach(list, function(item, key) {
        if (item.sender && (item.sender === uid)) {
          arr.push(item)
      }
  }, arr);
      return arr;
  });
}

function get(templateId) {
    return templates.$getRecord(templateId);
}

function add(template) {
    return templates.$add(template);
}

function remove(template) {
    templates.$remove(template);
}

return {
    getAll: getAll,
    get: get,
    add: add,
    remove: remove
};
}])

.factory("Auth", ["$firebaseAuth", function($firebaseAuth) {  
  return $firebaseAuth();
}])

.factory("User", ["$firebaseArray", "Auth", function($firebaseArray, Auth) {
  var usersRef = firebase.database().ref().child("users");

  var currentUserUid = 0;

  function getCurrent() {
    return $firebaseArray(usersRef).$loaded().then(function(list) {
      return list.$getRecord(getUid());
  }).catch(function(error) {
      console.log("Error:", error);
      return error;
  });
}

function setCurrent(uid) {
    currentUserUid = uid;
}

function getUid() {
  var auth = Auth.$getAuth();
    return  auth ? auth.uid : null;
}

function createProfile(profile) {
    usersRef.child(profile.uid).set(profile);
}

function updateProfile(profile) {

}

function deleteProfile(profileId) {

}

function isAuthenticated() {
    return Auth.$getAuth();
}

return {
    setCurrent: setCurrent,
    getCurrent: getCurrent,
    getUid: getUid,
    createProfile: createProfile,
    updateProfile: updateProfile,
    deleteProfile: deleteProfile,
    isAuthenticated: isAuthenticated
};
}])

.factory("DocumentNumber", ["$firebaseObject", "User", function($firebaseObject, User) {
  var docCounterRef = firebase.database().ref().child("docCounter");
  
  docCounter = $firebaseObject(docCounterRef);
  

  function getNew() {
    return User.getCurrent().then(function(user){
      return user.totalDocs + 1;
  }).catch(function(error) {
      console.log("Error:", error);
  });  ;
}

function setNew(val) {
    return docCounter.$loaded().then(function() {
      docCounter.$value = val;
      docCounter.$save();
  });
}

return {
    getNew: getNew,
    setNew: setNew
}
}])

.factory('ApiCommunicator', function($http) {

    function getPredictions(data) {
        var req = {
            method: 'POST',
            url: Config.restAPIs.predictions,
            data: JSON.stringify(data)
        }
        return $http(req).then(function(result) {
            return result.data;
        });
    }
    return {
      getPredictions: getPredictions
  }
})
