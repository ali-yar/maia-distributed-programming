angular.module('itncApp.controllers')
.controller('MammogramsCtrl', ['$scope', '$state', '$ionicPopup', '$ionicLoading', 'Templates', 'User', 
                      function($scope, $state, $ionicPopup, $ionicLoading, Templates, User) {
                        
  // With the new view caching in Ionic, Controllers are only called
  // when they are recreated or on app start, instead of every page change.
  // To listen for when this page is active (for example, to refresh data),
  // listen for the $ionicView.enter event:
  //
  //$scope.$on('$ionicView.enter', function(e) {
  //});

  
  showSpinner("Loading documents . . .");

  $scope.currentUserUid = User.getUid();

    Templates.getAll($scope.currentUserUid).then(function(list) {
      $scope.templates = list;
      hideSpinner();
    });

  $scope.remove = function(template) {
    var confirmPopup = $ionicPopup.confirm({
      title: 'Delete',
      template: 'Are you sure you want to delete this item?',
      okText: 'Yes'
    });
    confirmPopup.then(function(res) {
      if(res) {
        Templates.remove(template);
        $scope.refreshData();
      }
     });
  };

  $scope.refreshData = function() {
     showSpinner("Loading documents . . .");
     $scope.templates = [];
    Templates.getAll($scope.currentUserUid).then(function(list) {
      $scope.templates = list;
      hideSpinner();
    });
  }

  $scope.newTemplate = function() {
    $state.go("tab.mammogram-new");
  }

  $scope.filterItemsByUser = function (list) {
    var newList = [];
    console.log(list)
    angular.forEach(list, function(item, key) {
      if (item.sender.uid === $scope.currentUserUid) {
        this.push(item);
      }

    }, newList);
    return newList;
  }

  function showSpinner(msg) {
    $ionicLoading.show({
        template: msg
      });
  }

  function hideSpinner() {
     $ionicLoading.hide();
  }

  $scope.$on('$stateChangeStart', function(event, toState, toParams, fromState, fromParams){
     $scope.refreshData();
    });

 
}]);