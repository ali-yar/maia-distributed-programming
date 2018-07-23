var Utils = (function() {

	var getBase64 = function (file) {
		return new Promise((resolve, reject) => {
			const reader = new FileReader();
			reader.readAsDataURL(file);
			reader.onload = () => resolve(reader.result);
			reader.onerror = error => reject(error);
		});
	}


	return {
		getBase64: getBase64
	}

})();