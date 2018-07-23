package it.unicas.dpn.transferlearning;

import it.unicas.dpn.datahelpers.BreastMassDataSet;
import org.deeplearning4j.eval.Evaluation;
import org.deeplearning4j.eval.ROC;
import org.deeplearning4j.nn.conf.distribution.NormalDistribution;
import org.deeplearning4j.nn.conf.layers.Layer;
import org.deeplearning4j.nn.conf.layers.OutputLayer;
import org.deeplearning4j.nn.graph.ComputationGraph;
import org.deeplearning4j.nn.transferlearning.FineTuneConfiguration;
import org.deeplearning4j.nn.transferlearning.TransferLearning;
import org.deeplearning4j.nn.weights.WeightInit;
import org.deeplearning4j.optimize.listeners.ScoreIterationListener;
import org.deeplearning4j.util.ModelSerializer;
import org.deeplearning4j.zoo.ZooModel;
import org.deeplearning4j.zoo.model.VGG16;
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
import org.nd4j.linalg.schedule.ScheduleType;
import org.nd4j.linalg.schedule.StepSchedule;
import org.slf4j.Logger;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;
import java.util.Random;

public class TrainDenseLayers {
    private static final Logger log = org.slf4j.LoggerFactory.getLogger(TrainDenseLayers.class);

    protected static final long seed = 123;
    protected static Random rng = new Random(seed);

    protected static final int numClasses = 2;
    private static final String featureExtractionLayer = "block5_pool";

    private static int epochs = 30;
    private static int batchSize = 16;
    private static String optimizer = "adam";
    private static double learnRate = 0.0001;
    private static double momentum = 5e-5;
    private static double trainPerc = 80, testPerc = 10, validPerc = 10;
    private static boolean balanced = true;
    private static boolean normalizeData = true;
    private static boolean saveBestModel = false;
    private static String modelsDir = "C:\\Users\\berradaa\\Documents\\dpn\\models\\";

    private static int epochIdx = 0;
    private static double maxAUC = .0;

    public static void main(String [] args) throws IOException{

        log.info("Program started");

        // set params configuration
        //loadConfig(new File(System.getProperty("user.home")) + "/trainlastlayer_config.properties");

        // data normalization
        DataNormalization dataNorm = null;

        // dataset iterators
        BreastMassDataSet.setup(batchSize, balanced, trainPerc,testPerc,validPerc);
        DataSetIterator dataIterTrain = BreastMassDataSet.trainIterator(null, null);
        if (normalizeData) {
            dataNorm = new NormalizerStandardize();
            dataNorm.fit(dataIterTrain);
            dataIterTrain.setPreProcessor(dataNorm);
        }
        DataSetIterator dataIterTest = BreastMassDataSet.testIterator(null, dataNorm);
        DataSetIterator dataIterValid = BreastMassDataSet.validIterator(null, dataNorm);

        ComputationGraph vgg16Transfer = getModel();
        log.info(vgg16Transfer.summary());

        // print loss score at intervals
        vgg16Transfer.setListeners(new ScoreIterationListener(5));

        // initial evaluation of model
        evaluateModel(vgg16Transfer, dataIterValid, saveBestModel, false, dataNorm);

        log.info("Training started");

        while(epochIdx < epochs) {
            log.info("\n=========================> Current epoch: " + (epochIdx + 1));

            // train model
            dataIterTrain.reset();
            while(dataIterTrain.hasNext()) {
                vgg16Transfer.fit(dataIterTrain.next());
            }

            // evaluate on validation set
            if ((epochIdx+1) % 2 != 0){
                log.info("Evaluate model on validation set at epoch " + (epochIdx + 1) + " ....");
                evaluateModel(vgg16Transfer, dataIterValid, saveBestModel, false, dataNorm);
            }

            // evaluate on test set at last epoch or if training score is very low
            if ( ((epochIdx+1) == epochs) || (vgg16Transfer.score() - 5e-4 < 0 ) ) {
                log.info("Training finished");
                log.info("Final predictions on test set");
                evaluateModel(vgg16Transfer, dataIterTest, false, true, dataNorm);
                break;
            }

            // intermediate evaluation on test set
            if ((epochIdx+1) == 1 || (epochIdx+1) == 7 || (epochIdx+1)%9 == 0) {
                log.info("Intermediate evaluation on test set");
                evaluateModel(vgg16Transfer, dataIterTest, false, false, dataNorm);
            }

            epochIdx++;
        }

        log.info("Program finished");
    }

    public static ComputationGraph getModel() throws IOException {

        // import pre-trained VGG-16 based on Keras Model Zoo
        log.info("Loading pre-trained VGG-16 model...");
        ZooModel zooModel = new VGG16();
        ComputationGraph vgg16 = (ComputationGraph) zooModel.initPretrained();

//        log.info(vgg16.summary());

        // set the learning rate schedule
        ISchedule lrSchedule = new StepSchedule(ScheduleType.EPOCH, 0.01, 0.50, 5);

        // set the optimizer
        IUpdater updater;
        if (optimizer.equalsIgnoreCase("adam")) {
            updater = new Adam.Builder().learningRate(learnRate).learningRateSchedule(lrSchedule).build();
        } else if (optimizer.equalsIgnoreCase("sgd")) {
            updater = new Sgd.Builder().learningRate(learnRate).learningRateSchedule(lrSchedule).build();
        } else {
            updater = new Nesterovs(learnRate,momentum);
        }

        // set the fine tuning configuration (applicable to unfrozen layers only)
        FineTuneConfiguration fineTuneConf = new FineTuneConfiguration.Builder()
                .updater(updater)
                .activation(Activation.RELU)
                .weightInit(WeightInit.XAVIER)
                .dropOut(0.5)
                .seed(seed)
                .build();

        // set the output layer
        Layer outputLayer = new OutputLayer.Builder(LossFunctions.LossFunction.NEGATIVELOGLIKELIHOOD)
                .nIn(4096).nOut(numClasses)
                .weightInit(WeightInit.DISTRIBUTION)
                .dist(new NormalDistribution(0,0.2*(2.0/(4096+numClasses))))
                .activation(Activation.SOFTMAX).build();

        // construct the new model
       return new TransferLearning.GraphBuilder(vgg16)
                .fineTuneConfiguration(fineTuneConf)
                .setFeatureExtractor(featureExtractionLayer) //the specified layer and below are "frozen"
                .removeVertexKeepConnections("predictions") //replace the functionality of the final vertex
                .addLayer("predictions", outputLayer,"fc2")
                .setOutputs("predictions")
                .build();

    }

    private static void evaluateModel(ComputationGraph vgg16Transfer, DataSetIterator dataIter, boolean saveBestModel,
                                      boolean saveModel, DataNormalization dataNorm) throws IOException {

        Evaluation eval = new Evaluation();
        ROC roc = new ROC(0);

        // evaluate
        dataIter.reset();
        vgg16Transfer.doEvaluation(dataIter, eval, roc);

        // compute AUC and AUCPR
        double AUC = Double.parseDouble(String.format("%.4f", roc.calculateAUC()));
        double AUCPR = Double.parseDouble(String.format("%.4f", roc.calculateAUCPR()));

        // print scores
        log.info(eval.stats());
        log.info("ACC:\t" + Double.parseDouble(String.format("%.4f", eval.accuracy())));
        log.info("AUC:\t" + AUC);
        log.info("AUCPR:\t" + AUCPR);

        if (saveModel || (saveBestModel && (AUC > maxAUC))) {
            maxAUC = AUC;
            saveModel(vgg16Transfer, modelsDir, dataNorm);
        }
    }

    private static void saveModel(ComputationGraph model, String dir, DataNormalization dataNorm) throws IOException{
        String fullPath = dir + TrainDenseLayers.class.getSimpleName() + "_model.zip";
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
        optimizer = prop.getProperty("optimizer",optimizer);
        normalizeData = Boolean.parseBoolean(prop.getProperty("normalizeData",String.valueOf(normalizeData)));
        saveBestModel = Boolean.parseBoolean(prop.getProperty("saveBestModel",String.valueOf(saveBestModel)));
        modelsDir = prop.getProperty("modelsDir",modelsDir);

        input.close();

        log.info("Configuration properties loaded.");
    }

}
