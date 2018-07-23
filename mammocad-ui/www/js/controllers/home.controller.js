angular.module('itncApp.controllers').controller('HomeCtrl', ['$scope', '$state', 'currentUser', 'Auth', 'User', function($scope, $state, currentUser, Auth, User) {
	// check is user is authenticated
	if (User.isAuthenticated()) {
	  console.log("User session found.");
	} else {
	  console.log("No user session found.");
	 $state.go('intro'); // redirect to login page
	}

	$scope.user = currentUser;
	console.log($scope.user);

	$scope.logout = function() {
    	Auth.$signOut().then(function() {
 			console.log("Signed out");
 			$state.go('intro');
	    }).catch(function(error) {
	    	console.log(error);
	    });
  	};
}]);