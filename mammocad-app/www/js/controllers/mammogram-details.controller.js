angular.module('itncApp.controllers')
.controller('MammogramDetailCtrl', function($scope, $stateParams, Templates) {
	
	$scope.template = Templates.get($stateParams.id);

	$scope.showComment = function(comments, field) {
		if (comments && comments[field]) {
			msg = "From " + comments.reviewer.name + " ("+ comments.date + "):\n" + comments[field];
			alert(msg);
		}
	}
})
