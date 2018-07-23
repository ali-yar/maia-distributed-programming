package it.unicas.dpn.mammocad.service;

import it.unicas.dpn.mammocad.model.Prediction;
import it.unicas.dpn.mammocad.util.Utils;
import org.datavec.image.loader.NativeImageLoader;
import org.deeplearning4j.nn.graph.ComputationGraph;
import org.deeplearning4j.util.ModelSerializer;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.nd4j.linalg.dataset.api.preprocessor.Normalizer;
import org.nd4j.linalg.primitives.Pair;
import org.springframework.stereotype.Component;
import org.springframework.util.ResourceUtils;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

@Component
public class TrainedNetwork {
    // commented lines should be used instead if the model is of type ComputationGraph (e.g. transfer learning VGG16)

    private ComputationGraph model;
//    private MultiLayerNetwork model;
    private Normalizer normalizer;

    private int height;
    private int width;
    private int depth;

    public TrainedNetwork() throws IOException {
        super();

        File file = ResourceUtils.getFile("classpath:model.zip");
        Pair<ComputationGraph,Normalizer> p = ModelSerializer.restoreComputationGraphAndNormalizer(file, false);
        // Pair<MultiLayerNetwork,Normalizer> p = ModelSerializer.restoreMultiLayerNetworkAndNormalizer(file, false);
        model = p.getLeft();
        normalizer = p.getRight();

        height = 224; width = 224; depth = 3;
//        height = 224; width = 224; depth = 1;
    }

    public Prediction doPrediction(String imageCode) throws IOException {
        InputStream stream = Utils.base64ToStream(imageCode);

        NativeImageLoader loader = new NativeImageLoader(height, width, depth);

        INDArray image = loader.asMatrix(stream);

//        DataNormalization scaler = new VGG16ImagePreProcessor();
//        scaler.transform(image);

        INDArray output = model.outputSingle(false,image);
//        INDArray output = model.output(image);

        Prediction p = new Prediction(output.getFloat(0),output.getFloat(1));

        stream.close();

        return p;
    }
}
