angular.module('itncApp.controllers')

.controller('DiagnosisCtrl', ['$scope', '$state', '$ionicPopup', '$ionicLoading', 'ApiCommunicator', 'User', function($scope, $state, $ionicPopup, $ionicLoading, ApiCommunicator, User) {

	$scope.showCroppedWindow = false;
	$scope.showPredictions = false;

	$scope.predictions = {
		probabilityClass0: 0,
		probabilityClass1: 0
	}

	$scope.cropped = { source: "test" };

	$('#upload').on('change', function () {
		var input = this;
		if (input.files && input.files[0]) {
			var reader = new FileReader();
			reader.onload = function (e) {
	        	// bind new Image to Component
	        	$scope.$apply(function () {
	        		$scope.cropped.source = e.target.result;
	        		$scope.showCroppedWindow = true;
	        	});
	        }
	        reader.readAsDataURL(input.files[0]);
	    }
	});

	$scope.loadFile = function() {
		$("#upload").click();
	}

	$scope.diagnose = function() {
		showSpinner("Working on it..");
		var data = { imageData: $scope.cropped.image };
		ApiCommunicator.getPredictions(data).then(function(result) {
			if (result) {
				$scope.predictions.probabilityClass0 = (result.probabilityClass0*100).toFixed(2);
				$scope.predictions.probabilityClass1 = (result.probabilityClass1*100).toFixed(2);
				$scope.showPredictions = true;
			}
			hideSpinner();
		}).catch(function(error) {
        	console.log("Error:", error);
        	hideSpinner();
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

  	// $scope.$on('$stateChangeStart', function(event, toState, toParams, fromState, fromParams){
  		
  	// });
  }]);