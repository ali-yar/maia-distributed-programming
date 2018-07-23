package it.unicas.dpn.mammocad.controller;

import it.unicas.dpn.mammocad.model.Prediction;
import it.unicas.dpn.mammocad.service.TrainedNetwork;
import it.unicas.dpn.mammocad.util.Utils;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;
import java.io.IOException;
import java.util.Map;

@RestController
@RequestMapping("/predict")
public class PredictionController {

    @Autowired
    TrainedNetwork model;

    @RequestMapping("/test")
    public String test() {
        return "Success";
    }

    @CrossOrigin(origins = "http://localhost:9000")
    @PostMapping(value="/mass")
    public Prediction predictMass(@RequestBody String data) throws IOException {
        Map<String, Object> map = Utils.parseJSON(data);
        return model.doPrediction((String) map.get("imageData"));
    }
}
