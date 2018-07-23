package it.unicas.dpn.mammocad.util;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;

import java.io.*;
import java.util.Base64;
import java.util.HashMap;
import java.util.Map;

/**
 * Provide helper functions.
 */

public class Utils {

    public static InputStream base64ToStream(String code) {
        String[] splits = code.split(",",2); // removing unwanted header followed by ','
        if (splits.length == 2) {
            code = splits[1];
        }
        byte[] bytes = Base64.getDecoder().decode(code);
        return new ByteArrayInputStream(bytes);
    }

    public static Map<String, Object>  parseJSON(String jsonString) throws IOException {
        ObjectMapper objectMapper = new ObjectMapper();
        return objectMapper.readValue(jsonString, new TypeReference<HashMap<String,Object>>(){});
    }

//    public static String imageToBase64(String imagePath) {
//        File file = new File(imagePath);
//        try (FileInputStream imageInFile = new FileInputStream(file)) {
//            // Reading a Image file from file system
//            String base64Image = "";
//            byte imageData[] = new byte[(int) file.length()];
//            imageInFile.read(imageData);
//            base64Image = Base64.getEncoder().encodeToString(imageData);
//            return base64Image;
//        } catch (FileNotFoundException e) {
//            System.out.println("Image not found" + e);
//        } catch (IOException ioe) {
//            System.out.println("Exception while reading the Image " + ioe);
//        }
//        return null;
//    }

//    public static void base64ToImage(String code) {
//        try (FileOutputStream imageOutFile = new FileOutputStream(pathFile)) {
//            // Converting a Base64 String into Image byte array
//            byte[] imageByteArray = Base64.getDecoder().decode(base64Image);
//            imageOutFile.write(imageByteArray);
//        } catch (FileNotFoundException e) {
//            System.out.println("Image not found" + e);
//        } catch (IOException ioe) {
//            System.out.println("Exception while reading the Image " + ioe);
//        }
//    }

}
