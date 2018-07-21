package it.unicas.dpn.massdetectionservice;

import it.unicas.dpn.massdetectionservice.service.TrainedNetwork;
import org.datavec.image.loader.NativeImageLoader;
import org.deeplearning4j.nn.graph.ComputationGraph;
import org.deeplearning4j.util.ModelSerializer;
import org.nd4j.linalg.api.ndarray.INDArray;
import org.slf4j.Logger;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.context.ConfigurableApplicationContext;
import org.springframework.util.ResourceUtils;
import org.springframework.web.bind.annotation.RestController;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;

@SpringBootApplication
@RestController
public class MassDetectionServiceApplication {

	private static final Logger log = org.slf4j.LoggerFactory.getLogger(MassDetectionServiceApplication.class);

	public static void main(String[] args) throws IOException {

		ConfigurableApplicationContext context = SpringApplication.run(MassDetectionServiceApplication.class, args);

		TrainedNetwork model = context.getBean(TrainedNetwork.class);

//		System.out.println(model.doPrediction());

	}

}
