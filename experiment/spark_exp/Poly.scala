import org.apache.spark.ml.feature.PolynomialExpansion
import org.apache.spark.sql.SQLContext
import org.apache.spark.{SparkConf, SparkContext}
import org.apache.spark.mllib.linalg.{Vector, Vectors}
import org.apache.spark.mllib.regression.LabeledPoint

/**
  * Created by moon on 2016/5/5.
  */
object Poly {
  def main(args: Array[String]) {
    val conf = new SparkConf()
      .setAppName("polynomial")
      .set("spark.network.timeout", "15000000")
    val sc = new SparkContext(conf)
    val sqlContext = new SQLContext(sc)

    var start = System.currentTimeMillis()
    val df = sqlContext.read.format("libsvm").load("/32xreal-sim")
    System.out.println("load:" + (System.currentTimeMillis() - start))
    start = System.currentTimeMillis()
    val polynomialExpansion = new PolynomialExpansion()
      .setInputCol("features")
      .setOutputCol("polyFeatures")
      .setDegree(2)
    val polyDF = polynomialExpansion.transform(df)
    polyDF.foreach(x => 1)
    System.out.println("trans:" + (System.currentTimeMillis() - start))
    System.out.println("dim:" + polyDF.select("polyFeatures").rdd.take(10)(5).getAs[Vector](0).size)
  }
}
