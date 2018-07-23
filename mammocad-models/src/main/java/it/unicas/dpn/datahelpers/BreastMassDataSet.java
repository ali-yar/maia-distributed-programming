package it.unicas.dpn.datahelpers;

import org.apache.commons.io.FileUtils;
import org.datavec.api.io.filters.BalancedPathFilter;
import org.datavec.api.io.filters.PathFilter;
import org.datavec.api.io.filters.RandomPathFilter;
import org.datavec.api.io.labels.ParentPathLabelGenerator;
import org.datavec.api.records.impl.SequenceRecord;
import org.datavec.api.split.FileSplit;
import org.datavec.api.split.InputSplit;
import org.datavec.image.loader.BaseImageLoader;
import org.datavec.image.recordreader.ImageRecordReader;
import org.datavec.image.transform.ImageTransform;
import org.deeplearning4j.datasets.datavec.RecordReaderDataSetIterator;
import org.deeplearning4j.datasets.datavec.RecordReaderMultiDataSetIterator;
import org.nd4j.linalg.dataset.api.DataSetPreProcessor;
import org.nd4j.linalg.dataset.api.iterator.DataSetIterator;
import org.nd4j.linalg.dataset.api.iterator.MultiDataSetIterator;
import org.nd4j.util.ArchiveUtils;
import org.slf4j.Logger;

import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.util.Random;

/**
 * Automatically downloads the dataset
 * and uncompress it to the user's home directory
 */
public class BreastMassDataSet {
    private static final Logger log = org.slf4j.LoggerFactory.getLogger(BreastMassDataSet.class);

    private static final String MAIN_DIR = new File(System.getProperty("user.home")) + "/DPN_DataDir";
    private static final String DATA_URL = "https://github.com/ali-yar/maia-distributed-programming/raw/master/dataset-fit.zip";
    private static final String DATA_DIR = MAIN_DIR + "/dataset-fit";

    private static final String FILE_NAME = "dataset-fit.zip";

    private static final String [] allowedExtensions = BaseImageLoader.ALLOWED_FORMATS;
    private static final Random rng  = new Random(12345);

    private static int height = 224;
    private static int width = 224;
    private static int depth = 3;

    private static final int numClasses = 2;

    private static ParentPathLabelGenerator labelMaker = new ParentPathLabelGenerator();
    private static InputSplit trainData, validData, testData;
    private static int batchSize;

    public static DataSetIterator trainIterator(ImageTransform transform, DataSetPreProcessor preProcessor)
            throws IOException {
        return makeIterator(trainData, transform, preProcessor);

    }

    public static DataSetIterator validIterator(ImageTransform transform, DataSetPreProcessor preProcessor)
            throws IOException {
        return makeIterator(validData, transform, preProcessor);

    }

    public static DataSetIterator testIterator(ImageTransform transform, DataSetPreProcessor preProcessor)
            throws IOException {
        return makeIterator(testData, transform, preProcessor);

    }

    public static void setup(int batchSizeArg, boolean balanced, double trainPerc, double testPerc, double validPerc)
            throws IOException {
        setup(batchSizeArg, balanced, trainPerc, testPerc, validPerc, height, width, depth);
    }

    public static void setup(int batchSizeArg, boolean balanced, double trainPerc, double testPerc, double validPerc,
                             int newHeight, int newWidth, int newDepth)
            throws IOException {

        if (newHeight > 0) height = newHeight;
        if (newWidth > 0) width = newWidth;
        if (newDepth > 0) depth = newDepth;

        try {
            downloadAndUnzip();
        } catch (IOException e) {
            e.printStackTrace();
            log.error("IOException : ", e);
        }
        batchSize = batchSizeArg;
        File parentDir = new File(DATA_DIR);
        FileSplit filesInDir = new FileSplit(parentDir, allowedExtensions, rng);
        PathFilter pathFilter;
        if (balanced) {
            pathFilter = new BalancedPathFilter(rng, allowedExtensions, labelMaker);
        } else {
            pathFilter = new RandomPathFilter(rng, allowedExtensions);
        }
        if (trainPerc >= 100) {
            throw new IllegalArgumentException("Percentage of data set aside for training has to be less than 100%. " +
                    "Test percentage = 100 - training percentage - validation percentage, has to be greater than 0");
        }

        InputSplit[] filesInDirSplit = filesInDir.sample(pathFilter, trainPerc, testPerc, validPerc);
        trainData = filesInDirSplit[0];
        testData = filesInDirSplit[1];
        validData = filesInDirSplit[2];

        log.info("Total train samples: " + trainData.length());
        log.info("Total test samples: " + testData.length());
        log.info("Total validation samples: " + validData.length());
    }

    private static DataSetIterator makeIterator(InputSplit split, ImageTransform transform,
                                                DataSetPreProcessor preProcessor) throws IOException {

        if (split.length() == 0) {
            return null;
        }
        split.reset();
        ImageRecordReader recordReader = new ImageRecordReader(height,width, depth,labelMaker);
        recordReader.initialize(split,transform);
        DataSetIterator iterator = new RecordReaderDataSetIterator(recordReader, batchSize, 1, numClasses);
        if (preProcessor != null) {
            iterator.setPreProcessor(preProcessor);
        }
        return iterator;
    }

    public static void downloadAndUnzip() throws IOException {
        if ((new File(DATA_DIR)).exists()) {
            log.info("Found the dataset in " + DATA_DIR);
            return ;
        }
        File rootFile = new File(MAIN_DIR);
        if (!rootFile.exists()) {
            rootFile.mkdir();
        }
        File zipFile = new File(MAIN_DIR, FILE_NAME);
        if (!zipFile.isFile()) {
            log.info("Downloading the dataset from " + DATA_URL + "...");
            FileUtils.copyURLToFile(new URL(DATA_URL), zipFile);
        }
        ArchiveUtils.unzipFileTo(zipFile.getAbsolutePath(), rootFile.getAbsolutePath());
    }
}