var Config = (function() {
	//var host = "http://localhost:8080";
	var host = "http://localhost:8080";

	var restAPIs = {
		"predictions": host + "/predict/mass"
	}

	return {
		restAPIs: restAPIs
	}
})();