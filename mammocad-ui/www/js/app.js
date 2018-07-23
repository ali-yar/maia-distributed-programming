angular.module('itncApp', ['ionic', 'firebase', 'itncApp.controllers', 'itncApp.services'])

.run(function($ionicPlatform) {
  $ionicPlatform.ready(function() {
    // Hide the accessory bar by default (remove this to show the accessory bar above the keyboard
    // for form inputs)
    if (window.cordova && window.cordova.plugins && window.cordova.plugins.Keyboard) {
      cordova.plugins.Keyboard.hideKeyboardAccessoryBar(true);
      cordova.plugins.Keyboard.disableScroll(true);

    }
    if (window.StatusBar) {
      // org.apache.cordova.statusbar required
      StatusBar.styleDefault();
    }
  });
})

.config(function($stateProvider, $urlRouterProvider) {

  $stateProvider

  .state('intro', {
    url: '/',
    templateUrl: 'templates/intro.html',
    controller: 'IntroCtrl'
  })

  .state('tab', {
    url: '/tab',
    abstract: true,
    templateUrl: 'templates/tabs.html',
    resolve: {
      'currentUser': 
        ["User", "$state", function(User, $state) {
          return User.getCurrent().then(function(user){
            return user;
          }).catch(function(error) {
            console.log("Error:", error);
            $state.go('intro');
          });  
        }]
    }
  })

  .state('tab.home', {
    url: '/home',
    views: {
      'tab-home': {
        templateUrl: 'templates/tab-home.html',
        controller: 'HomeCtrl'
      }
    },
    resolve: {
      'currentUser': 
        ["User", "$state", function(User, $state) {
          return User.getCurrent().then(function(user){
            return user;
          }).catch(function(error) {
            console.log("Error:", error);
            $state.go('intro');
          });  
        }]
    }
  })

  .state('tab.mammograms', {
      url: '/mammograms',
      views: {
        'tab-mammograms': {
          templateUrl: 'templates/tab-mammograms.html',
          controller: 'MammogramsCtrl'
        }
      },
      resolve: {
      'currentUser': 
        ["User", "$state", function(User, $state) {
          return User.getCurrent().then(function(user){
            return user;
          }).catch(function(error) {
            console.log("Error:", error);
            $state.go('intro');
          });  
        }]
    }
    })
    .state('tab.mammogram-detail', {
      url: '/mammograms/:id',
      views: {
        'tab-mammograms': {
          templateUrl: 'templates/tab-mammograms-detail.html',
          controller: 'MammogramDetailCtrl'
        }
      }
    })
    .state('tab.mammogram-new', {
      url: '/mammograms/new',
      views: {
        'tab-mammograms': {
          templateUrl: 'templates/tab-mammograms-new.html',
          controller: 'MammogramNewCtrl'
        }
      },
      resolve: {
      'currentUser': 
        ["User", "$state", function(User, $state) {
          return User.getCurrent().then(function(user){
            //console.log(user);
            return user;
          }).catch(function(error) {
            console.log("Error:", error);
            $state.go('intro');
          });  
        }]
    }
    })

    .state('tab.diagnosis', {
      url: '/diagnosis',
      views: {
        'tab-diagnosis': {
          templateUrl: 'templates/tab-diagnosis.html',
          controller: 'DiagnosisCtrl'
        }
      }
    })

  $urlRouterProvider.otherwise('/');

});

angular.module('itncApp.controllers', ['angularCroppie']);

angular.module('itncApp.services', []);
