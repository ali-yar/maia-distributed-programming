package it.unicas.dpn.massdetectionservice.service;

import it.unicas.dpn.massdetectionservice.MassDetectionServiceApplication;
import it.unicas.dpn.massdetectionservice.model.Prediction;
import it.unicas.dpn.massdetectionservice.util.Utils;
import org.datavec.image.loader.NativeImageLoader;
import org.deeplearning4j.nn.graph.ComputationGraph;
import org.deeplearning4j.util.ModelSerializer;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.slf4j.Logger;
import org.springframework.stereotype.Component;
import org.springframework.util.ResourceUtils;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

@Component
public class TrainedNetwork {

    private ComputationGraph model;

    public TrainedNetwork() throws IOException {
        super();

        File file = ResourceUtils.getFile("classpath:trainlastlayer_model.zip");
        model = ModelSerializer.restoreComputationGraph(file);
    }

    public Prediction doPrediction(String imageCode) throws IOException {
        InputStream stream = Utils.base64ToStream(imageCode);

        NativeImageLoader loader = new NativeImageLoader(224, 224, 3);

        INDArray image = loader.asMatrix(stream);

        INDArray output = model.outputSingle(false,image);

        Prediction p = new Prediction(output.getFloat(0),output.getFloat(1));

        stream.close();

        return p;
    }
}
