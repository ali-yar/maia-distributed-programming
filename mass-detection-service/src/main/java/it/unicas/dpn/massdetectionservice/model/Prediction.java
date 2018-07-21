package it.unicas.dpn.massdetectionservice.model;

public class Prediction {

    private float probabilityClass0;
    private float probabilityClass1;
    private int predictedClass;

    public Prediction() {
    }

    public Prediction(float probabilityClass0, float probabilityClass1) {
        this.probabilityClass0 = probabilityClass0;
        this.probabilityClass1 = probabilityClass1;
        this.predictedClass = ((probabilityClass1 - probabilityClass0) > 0) ? 1 : 0;
    }

    public float getProbabilityClass0() {
        return probabilityClass0;
    }

    public void setProbabilityClass0(float probabilityClass0) {
        this.probabilityClass0 = probabilityClass0;
    }

    public float getProbabilityClass1() {
        return probabilityClass1;
    }

    public void setProbabilityClass1(float probabilityClass1) {
        this.probabilityClass1 = probabilityClass1;
    }

    public int getPredictedClass() {
        return predictedClass;
    }

    public void setPredictedClass(int predictedClass) {
        this.predictedClass = predictedClass;
    }

    @Override
    public String toString() {
        return "Prediction{" +
                "probabilityClass0=" + probabilityClass0 +
                ", probabilityClass1=" + probabilityClass1 +
                ", predictedClass=" + predictedClass +
                '}';
    }
}
