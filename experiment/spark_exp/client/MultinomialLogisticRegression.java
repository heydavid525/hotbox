package test.lr.spark;

import scala.Tuple2;

import org.apache.spark.api.java.*;
import org.apache.spark.api.java.function.Function;
import org.apache.spark.api.java.function.VoidFunction;
import org.apache.spark.mllib.classification.LogisticRegressionModel;
import org.apache.spark.mllib.classification.LogisticRegressionWithLBFGS;
import org.apache.spark.mllib.evaluation.MulticlassMetrics;
import org.apache.spark.mllib.linalg.SparseVector;
import org.apache.spark.mllib.linalg.Vector;
import org.apache.spark.mllib.regression.LabeledPoint;
import org.apache.spark.mllib.util.MLUtils;
import org.apache.spark.SparkConf;
import org.apache.spark.SparkContext;

import java.sql.SQLException;
import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.Statement;
import java.sql.DriverManager;
import java.util.ArrayList;
import java.util.List;

import org.apache.spark.sql.DataFrame;
import org.apache.spark.sql.Row;
import org.apache.spark.sql.hive.*;

public class MultinomialLogisticRegression {
  public static void main(String[] args) {
    SparkConf conf = new SparkConf().setAppName("LR on url_combined");
    conf.set("spark.network.timeout", "1200000");
    conf.set("spark.driver.maxResultSize", "30g");
    long start = 0, end = 0;
    SparkContext sc = new SparkContext(conf);
    JavaSparkContext jsc = new JavaSparkContext(sc);
    HiveContext hc = new HiveContext(sc);
    start = System.currentTimeMillis();
    DataFrame df = hc.table("url_combined");
    end = System.currentTimeMillis();
    System.out.println("hc load data: " + (end - start));
    start = System.currentTimeMillis();
    System.out.println("size: " + df.count());    
    end = System.currentTimeMillis();
    System.out.println("count time: " + (end - start));
    
    start = System.currentTimeMillis();
    JavaRDD<LabeledPoint> training = df.javaRDD().map(new Function<Row, LabeledPoint>() {
	  public LabeledPoint call(Row row) {
		  String record = row.getString(0);
		  String[] eles = record.split(" ");
		  double lable = Double.parseDouble(eles[0]);
		  if(lable == -1)
			  lable = 0;
		  List<Integer> indicesList = new ArrayList<Integer>();
		  List<Double> valuesList = new ArrayList<Double>();
		  for(int i = 1; i < eles.length; i++){
			  String[] feature = eles[i].split(":");
			  indicesList.add(Integer.parseInt(feature[0])-1);
			  valuesList.add(Double.parseDouble(feature[1]));
		  }
		  int[] indices = new int[indicesList.size()];
		  double[] values = new double[valuesList.size()];
		  for(int i = 0; i < indicesList.size(); i++)
			  indices[i] = indicesList.get(i);
		  for(int i = 0; i < valuesList.size(); i++)
			  values[i] = valuesList.get(i);
		  Vector features = new SparseVector(3231961, indices, values); 
		  return new LabeledPoint(lable, features);
	  }
	});
    training.count();
    end = System.currentTimeMillis();
    System.out.println("transform: " + (end - start));

    // Run training algorithm to build the model.
    start = System.currentTimeMillis();
    final LogisticRegressionModel model = new LogisticRegressionWithLBFGS()
      .setNumClasses(2)
      .run(training.rdd());
    end = System.currentTimeMillis();
    System.out.println("training: " + (end - start));
  }
}
