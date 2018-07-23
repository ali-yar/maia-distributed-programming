angular.module('itncApp.controllers')
.controller('MammogramNewCtrl', ['$scope', '$state', '$stateParams', 'currentUser', 'DocumentNumber', 'Templates',
						 function($scope, $state, $stateParams, currentUser, DocumentNumber, Templates) {
						 	
	$scope.formData = {};
	$scope.tmp = { date: new Date()};

	document.getElementById('fileinput').addEventListener('change', function(){
		Utils.getBase64(this.files[0]).then(data => $scope.formData.data = data);
	}, false);

	$scope.submit = function() {
		var date = $scope.tmp.date;
		$scope.formData.date = [date.getDate(), date.getMonth()+1, date.getFullYear()].join('/');
		$scope.formData.sender = currentUser.$id;
		console.log($scope.formData)
		Templates.add($scope.formData).then(function() {
			DocumentNumber.setNew($scope.formData.docNumber).then(function() {
				$state.go("tab.mammograms");
			});
		});
	}

}]);
