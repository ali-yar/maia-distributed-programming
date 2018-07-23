package it.unicas.dpn.mammocad;

import it.unicas.dpn.mammocad.service.TrainedNetwork;
import org.slf4j.Logger;
import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.context.ConfigurableApplicationContext;
import org.springframework.web.bind.annotation.RestController;

import java.io.IOException;

@SpringBootApplication
@RestController
public class MainServiceApplication {

	private static final Logger log = org.slf4j.LoggerFactory.getLogger(MainServiceApplication.class);

	public static void main(String[] args) throws IOException {

		ConfigurableApplicationContext context = SpringApplication.run(MainServiceApplication.class, args);

		TrainedNetwork model = context.getBean(TrainedNetwork.class);

	}

}
