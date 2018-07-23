package it.unicas.dpn.convolution;

import it.unicas.dpn.datahelpers.BreastMassDataSet;
import org.deeplearning4j.datasets.datavec.RecordReaderDataSetIterator;
import org.deeplearning4j.eval.Evaluation;
import org.deeplearning4j.eval.EvaluationBinary;
import org.deeplearning4j.eval.ROC;
import org.deeplearning4j.eval.ROCBinary;
import org.deeplearning4j.nn.conf.NeuralNetConfiguration;
import org.deeplearning4j.nn.conf.inputs.InputType;
import org.deeplearning4j.nn.conf.layers.*;
import org.deeplearning4j.nn.multilayer.MultiLayerNetwork;
import org.deeplearning4j.nn.weights.WeightInit;
import org.deeplearning4j.optimize.listeners.ScoreIterationListener;
import org.deeplearning4j.util.ModelSerializer;
import org.nd4j.linalg.activations.Activation;
import org.nd4j.linalg.dataset.api.iterator.DataSetIterator;
import org.nd4j.linalg.dataset.api.preprocessor.DataNormalization;
import org.nd4j.linalg.dataset.api.preprocessor.NormalizerStandardize;
import org.nd4j.linalg.learning.config.Adam;
import org.nd4j.linalg.learning.config.IUpdater;
import org.nd4j.linalg.learning.config.Nesterovs;
import org.nd4j.linalg.learning.config.Sgd;
import org.nd4j.linalg.lossfunctions.LossFunctions;
import org.nd4j.linalg.schedule.ISchedule;
import org.nd4j.linalg.schedule.MapSchedule;
import org.nd4j.linalg.schedule.ScheduleType;
import org.slf4j.Logger;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Properties;

public class Custom {
    private static final Logger log = org.slf4j.LoggerFactory.getLogger(Custom.class);

    protected static final long seed = 123;
    protected static final int numClasses = 2;

    private static int epochs = 10;
    private static int batchSize = 16;
    private static String lossFunction = "default";
    private static String optimizer = "adam";
    private static double learnRate = 0.001;
    private static double momentum = 5e-5;
    private static double trainPerc = 80, testPerc = 10, validPerc = 10;
    private static boolean balanced = true;
    private static boolean normalizeData = false;
    private static boolean saveBestModel = false;
    private static String modelsDir = "C:\\Users\\berradaa\\Documents\\dpn\\models\\";

    private static int epochIdx = 0;
    private static double maxAUC = .0;

    private static int height = 224;
    private static int width = 224;
    private static int depth = 1;

    public static void main(String [] args) throws IOException {

        log.info("Program started");

        // set params configuration
        //loadConfig(new File(System.getProperty("user.home")) + "/trainlastlayer_config.properties");

        // data normalization
        DataNormalization dataNorm = null;

        // dataset iterators
        BreastMassDataSet.setup(batchSize, balanced, trainPerc, testPerc, validPerc, height, width, depth);
        DataSetIterator dataIterTrain = BreastMassDataSet.trainIterator(null, null);
        if (normalizeData) {
            dataNorm = new NormalizerStandardize();
            dataNorm.fit(dataIterTrain);
            dataIterTrain.setPreProcessor(dataNorm);
        }
        DataSetIterator dataIterTest = BreastMassDataSet.testIterator(null, dataNorm);
        DataSetIterator dataIterValid = BreastMassDataSet.validIterator(null, dataNorm);

        // build the model
        MultiLayerNetwork network = getModel();
        network.init();

        log.info(network.summary());

        // print loss score at intervals
        network.setListeners(new ScoreIterationListener(5));

        log.info("Training started");

        while(epochIdx < epochs) {
            log.info("===============================> Current epoch: " + (epochIdx + 1));

            // train model
            dataIterTrain.reset();
            while(dataIterTrain.hasNext()) {
                network.fit(((RecordReaderDataSetIterator) dataIterTrain).next());
            }

            // evaluate on validation set at certain epochs
            if ((epochIdx+1) % 2 != 0){
                log.info("Evaluation on validation set at epoch " + (epochIdx + 1) + " ....");
                evaluateModel(network, dataIterValid, saveBestModel, dataNorm);
            }

            // evaluate on test set at last epoch
            if ((epochIdx+1) == epochs) {
                log.info("Training finished");
                log.info("Final predictions on test set");
                evaluateModel(network, dataIterTest, true, dataNorm);
                break;
            }

            // intermediate evaluation on test set
            if ((epochIdx+1) == 1 || (epochIdx+1) == 7 || (epochIdx+1)%9 == 0) {
                log.info("Intermediate evaluation on test set");
                evaluateModel(network, dataIterTest, false, dataNorm);
            }

            epochIdx++;
        }

        log.info("Program finished");
    }

    public static MultiLayerNetwork getModel() {
        ArrayList<Layer> layers = getModelLayers();

        ISchedule lrSchedule = new MapSchedule.Builder(ScheduleType.EPOCH)
                .add(0, 0.001)
                .add(3, 0.0005)
                .add(5, 0.0001)
                .add(7, 0.00005)
                .add(9, 0.00001)
                .add(11, 0.000005)
                .add(20, 0.000001)
                .add(25, 0.0000005).build();


        // set the optimizer
        IUpdater updater;
        if (optimizer.equalsIgnoreCase("adam")) {
            updater = new Adam.Builder().learningRate(learnRate).learningRateSchedule(lrSchedule).build();
        } else if (optimizer.equalsIgnoreCase("sgd")) {
            updater = new Sgd.Builder().learningRate(learnRate).build();
        } else {
            updater = new Nesterovs(learnRate,5e-5);
        }

        // general configuration
        NeuralNetConfiguration.ListBuilder builder = new NeuralNetConfiguration.Builder()
                .seed(seed)
                //.l2(0.005)
                .weightInit(WeightInit.XAVIER)
                .updater(updater)
                .list()
                .setInputType(InputType.convolutional(height, width, depth))
                .backprop(true).pretrain(false);

        // add the layers
        int i = 0;
        for (Layer l : layers) {
            builder.layer(i++,l);
        }

        // build and return the network
        return new MultiLayerNetwork(builder.build());
    }

    public static ArrayList<Layer> getModelLayers() {

        LossFunctions.LossFunction loss;
        if (lossFunction.equalsIgnoreCase("crossentropy")) {
            loss = LossFunctions.LossFunction.MCXENT;
        } else {
            loss = LossFunctions.LossFunction.NEGATIVELOGLIKELIHOOD;
        }

        ArrayList<Layer> layers = new ArrayList<Layer>();

        // Convolution layer with 32 filters
        layers.add(new ConvolutionLayer.Builder().name("Conv1").nIn(depth).nOut(32).kernelSize(new int[]{3, 3})
                .activation(Activation.RELU).build());

        // Convolution layer with 32 filters
        layers.add(new ConvolutionLayer.Builder().name("Conv2").nOut(32).kernelSize(new int[]{3, 3})
                .activation(Activation.RELU).build());

        // Max pooling layer
        layers.add(new SubsamplingLayer.Builder().name("MaxPool1").kernelSize(new int[]{2,2}).build());

        // Batch normalization layer
        layers.add(new BatchNormalization.Builder().name("BatchNorm1").build());

        // Convolution layer with 32 filters
        layers.add(new ConvolutionLayer.Builder().name("Conv3").nOut(32).kernelSize(new int[]{3, 3})
                .activation(Activation.RELU).build());

        // Convolution layer with 32 filters
        layers.add(new ConvolutionLayer.Builder().name("Conv4").nOut(32).kernelSize(new int[]{3, 3})
                .activation(Activation.RELU).build());

        // Max pooling layer
        layers.add(new SubsamplingLayer.Builder().name("MaxPool2").kernelSize(new int[]{2,2}).build());

        // Batch normalization layer
        layers.add(new BatchNormalization.Builder().name("BatchNorm1").build());

        // Fully connected layer
        layers.add(new DenseLayer.Builder().name("FC1").nOut(256).activation(Activation.RELU).dropOut(0.5).build());

        // Fully connected layer
        layers.add(new DenseLayer.Builder().name("FC2").nOut(256).activation(Activation.RELU).dropOut(0.5).build());

        // Predictions layer
        layers.add(new OutputLayer.Builder().name("FC3").nOut(numClasses).activation(Activation.SOFTMAX)
                .lossFunction(loss).build());

        return layers;
    }

    private static double evaluateModel(MultiLayerNetwork vgg16Transfer, DataSetIterator dataIter, boolean saveBestModel,
                                        DataNormalization dataNorm)
            throws IOException {

        Evaluation eval = new Evaluation();
        ROC roc = new ROC(5);

        EvaluationBinary eval2 = new EvaluationBinary();
        ROCBinary roc2 = new ROCBinary(0);

        // evaluate
        dataIter.reset();
        vgg16Transfer.doEvaluation(dataIter, eval, roc);

        // compute AUC and AUCPR
        double AUC = Double.parseDouble(String.format("%.5f", roc.calculateAUC()));
        double AUCPR = Double.parseDouble(String.format("%.5f", roc.calculateAUCPR()));

        // print scores
        log.info(eval.stats());
        log.info("ACC:\t" + Double.parseDouble(String.format("%.4f", eval.accuracy())));
        log.info("AUC:\t" + AUC);
        log.info("AUCPR:\t" + AUCPR);
        log.info("\n" + eval.confusionToString());

        if (saveBestModel && (AUC > maxAUC)) {
            maxAUC = AUC;
            saveModel(vgg16Transfer, modelsDir, dataNorm);
        }

        return maxAUC;
    }


    private static void saveModel(MultiLayerNetwork model, String dir, DataNormalization dataNorm) throws IOException{
        String fullPath = dir + Custom.class.getSimpleName() + "_model.zip";
        log.info("Saving model to: " + fullPath);
        ModelSerializer.writeModel(model, new File(fullPath), true, dataNorm);
    }

    private static void loadConfig(String propFile) throws IOException {
        if (!(new File(propFile)).exists()) {
            log.info("No configuration properties file found. Using default params.");
            return ;
        }
        Properties prop = new Properties();
        InputStream input = new FileInputStream(propFile);

        prop.load(input);

        epochs = Integer.parseInt(prop.getProperty("epochs",String.valueOf(epochs)));
        batchSize = Integer.parseInt(prop.getProperty("batchSize",String.valueOf(batchSize)));
        learnRate = Double.parseDouble(prop.getProperty("learnRate",String.valueOf(learnRate)));
        momentum = Double.parseDouble(prop.getProperty("momentum",String.valueOf(momentum)));
        trainPerc = Double.parseDouble(prop.getProperty("trainPerc",String.valueOf(trainPerc)));
        testPerc = Double.parseDouble(prop.getProperty("testPerc",String.valueOf(testPerc)));
        balanced = Boolean.parseBoolean(prop.getProperty("balanced",String.valueOf(balanced)));
        validPerc = Double.parseDouble(prop.getProperty("validPerc",String.valueOf(validPerc)));
        lossFunction = prop.getProperty("lossFunction",lossFunction);
        optimizer = prop.getProperty("optimizer",optimizer);
        normalizeData = Boolean.parseBoolean(prop.getProperty("normalizeData",String.valueOf(normalizeData)));
        saveBestModel = Boolean.parseBoolean(prop.getProperty("saveBestModel",String.valueOf(saveBestModel)));
        modelsDir = prop.getProperty("modelsDir",modelsDir);

        input.close();

        log.info("Configuration properties loaded.");
    }

}
