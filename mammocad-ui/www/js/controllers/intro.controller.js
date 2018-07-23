angular.module('itncApp.controllers')
.controller('IntroCtrl', ['$scope', '$ionicSlideBoxDelegate', '$ionicLoading', '$timeout', '$state', 'Auth', 'User', function($scope, $ionicSlideBoxDelegate, $ionicLoading, $timeout, $state, Auth, User) {
	// check is user is authenticated
	var user = User.isAuthenticated();
	if (user) {
	  console.log("User session found.");
	  User.setCurrent(user.uid);
	  $state.go('tab.home'); // redirect to home page
	} else {
	  console.log("No user session found.");
	}


 	$scope.isNewAccount = false;
 	$scope.formTitle = 'Login';
 	$scope.form = {};

	// for developent only
 	$scope.form.email = "aliberrada@gmail.com";
 	$scope.form.password = "letmein12345";

 	$scope.data = {};
	$scope.options = {
		loop: false,
		effect: 'coverflow',
		speed: 300
	};

	$scope.toggleIsNewAccount = function() {
 		$scope.isNewAccount = !$scope.isNewAccount;
 		if($scope.isNewAccount) {
 			$scope.formTitle = 'Create Account';
 		} else {
 			$scope.formTitle = 'Login';
 		}
 	}

 	$scope.submit = function() {
 		showSpinner("Sending...");
 		var formInvalid = validateForm($scope.form);
 		if (formInvalid) {
 			hideSpinner();
 			displayMsg(formInvalid, 500);
 		} else if($scope.isNewAccount) {
 			createUser($scope.form);
 		} else {
 			loginUser($scope.form);
 		}
 	}

 	function validateForm(form) {
 		var error = null;
 		if (form.password && form.retypePassword && form.password !== form.retypePassword) {
 			error = "Form error: Passwords mismatched.";
 		}
 		return error;
 	}

 	function createUser(form) {
		$scope.message = null;
		$scope.error = null;

		Auth.$createUserWithEmailAndPassword(form.email,form.password).then(function(userData) {
			console.log("User created with uid: ", userData.uid);
			createUserProfile(userData.uid, form);
		}).catch(function(error) {
			hideSpinner();
			console.log(error);
			var msg;
			switch (error.code) {
		    	case "EMAIL_TAKEN":
			        msg = "The new user account cannot be created because the email is already in use.";
			        break;
		     	 case "INVALID_EMAIL":
			        msg = "The specified email is not a valid email.";
			        break;
		     	default:
		        	msg = "Error creating user: " + error;
		    }
			displayMsg(msg, 500);
		});
 	}

 	function loginUser(form) {
	  Auth.$signInWithEmailAndPassword(
 			form.email,
 			form.password
 		).then(function(authData) {
 			setUserProfile(authData.uid);
	    }).catch(function(error) {
	    	console.log(error);
	    	hideSpinner();
	    	displayMsg(error, 500);
	    });
 	}

 	function showSpinner(msg) {
 		$ionicLoading.show({
	      template: msg
	    });
 	}

 	function hideSpinner() {
 		 $ionicLoading.hide();
 	}

 	function createUserProfile(uid, form) {
 		var profile = {
			'uid': uid,
			'name': form.name,
			'staffId': form.staffId,
			'affiliation': form.affiliation,
			'email': form.email
		};
 		User.createProfile(profile);
 		hideSpinner();
 		$scope.toggleIsNewAccount();
		displayMsg("Account created successfully! You can login now.", 500);
 	}

 	function setUserProfile(uid) {
 		User.setCurrent(uid);
 		hideSpinner();
 		$scope.data.slider.slidePrev();
 		$scope.startApp();
 	}

 	function displayMsg(msg, wait) {
 		if (!wait) {
 			wait = 10;
 		}
 		$timeout(function(){
 			alert(msg);
 		}, wait);
 	}

	$scope.startApp = function() {
    	$state.go('tab.home');
  	};

  	$scope.showLoginSlide = function() {
  		$scope.data.slider.slideNext();
  	};

	// $scope.$watch('data.slider', function(nv, ov) {
	// 	$scope.slider = $scope.data.slider;
	// 	console.log("$scope.slider: ", $scope.slider)
	// });
}]);