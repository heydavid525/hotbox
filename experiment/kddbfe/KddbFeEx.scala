import java.io.FileInputStream

import opennlp.tools.tokenize.{TokenizerME, TokenizerModel}
import org.apache.spark.{SparkConf, SparkContext}
import org.apache.spark.mllib.clustering.{KMeans, KMeansModel}
import org.apache.spark.mllib.linalg.Vectors
import org.apache.spark.sql.DataFrame

/**
  * Created by zhangyy on 2016/4/12.
  */
object KddbFeEx {
  val tokenizerModel = new TokenizerModel(new FileInputStream("/root/en-token.bin"))

  /**
    * collect tokens from column
    * @param df
    * @param columnName
    * @return
    */
  def collect_tokens(df: DataFrame, columnName: String): Map[String, Int] = {
    df.select(columnName).flatMap(row => {
      new TokenizerME(tokenizerModel).tokenize(row(0).toString.toLowerCase.replace("~", " "))
    }).distinct().collect().view.zipWithIndex.toMap
  }

  /**
    * train KMeans model
    * @param df
    * @param tokens
    * @param columnName
    * @param k
    * @return
    */
  def train_model(df: DataFrame, tokens: Map[String, Int], columnName: String, k: Int): KMeansModel = {
    val trainData = df.select(columnName)
      .map(
        row => {
          val tokenizer = new TokenizerME(tokenizerModel)
          val indexes = tokenizer.tokenize(row(0).toString.toLowerCase.replace("~", " ")).map(token => tokens.get(token).get).distinct
          Vectors.sparse(tokens.size, indexes, Array.fill(indexes.length)(1))
        }
      ).cache()
    KMeans.train(trainData, k, 100, 1, "random")
  }

  /**
    * feature engineering on columns "KC(SubSkills)", "KC(KTracedSkills)" and remove columns "KC(SubSkills)", "KC(KTracedSkills)", "Row", "Correct First Attempt"
    * then saves to hdfs in csv format
    * @param args
    */
  def main(args: Array[String]) {
    val conf = new SparkConf()
      .setAppName("Kddb FE Application Ex")
      .set("spark.network.timeout", "15000000")
      .set("spark.driver.maxResultSize", "150g")
      .set("spark.driver.memory", "150g")
      .set("spark.executor.memory", "150g")
      .set("spark.worker.memory", "150g")
    // .setMaster("local[4]")
    val sc = new SparkContext(conf)
    val sqlContext = new org.apache.spark.sql.SQLContext(sc)

    val df = sqlContext.read.format("com.databricks.spark.csv")
      .option("delimiter", "\t")
      .option("header", "true")
      .option("inferSchema", "true")
      .load("kddb_output/out_to_binarize.csv") // /kddb_output/out.csv/part-00000
      .repartition(100)
    val columns = Array("KC(SubSkills)", "KC(KTracedSkills)") // "Problem Name", "Step Name"
    val columns2Drop = Array("KC(SubSkills)", "KC(KTracedSkills)", "Row", "Correct First Attempt") // "Problem Name", "Step Name"
    val dfColMap = df.columns.view.zipWithIndex.toMap
    val dfColIds = columns.map(col => dfColMap.get(col).get)
    val labelColId = dfColMap.get("Correct First Attempt").get
    val dfCols = df.columns
    val staticCols = dfCols.filter(!columns2Drop.contains(_))
    val staticColIds = staticCols.map(dfCols.indexOf(_))
    val tokenMaps = columns.map(col => collect_tokens(df, col))
    //    val model0 = train_model(df, tokenMaps(2), columns(2), 1000)
    //    val model1 = train_model(df, tokenMaps(3), columns(3), 2000)
    //    model0.save(sc, "kddb_output/kddb-kmeans-model0")
    //    model1.save(sc, "kddb_output/kddb-kmeans-model1")
    //    val model0 = KMeansModel.load(sc, "kddb_output/kddb-kmeans-model0")
    //    val model1 = KMeansModel.load(sc, "kddb_output/kddb-kmeans-model1")
    val staticSchema = staticCols.view.zipWithIndex.map(w => w._2 + ": " + w._1).mkString("\n")
    val from0 = staticCols.length
    val to0 = from0 + tokenMaps(0).size
    val kcsSchema = "[" + from0 + ", " + to0 + "): KC(SubSkills)"
    val from1 = to0
    val to1 = from1 + tokenMaps(1).size
    val kckSchema = "[" + from1 + ", " + to1 + "): KC(KTracedSkills)"
    val schema = staticSchema + "\n" + kcsSchema + "\n" + kckSchema
    println(schema)
    val outputRdd = df.map(
      row => {
        val tokenizer = new TokenizerME(tokenizerModel)
        val indices0 = tokenizer.tokenize(row(dfColIds(0)).toString.toLowerCase.replace("~", " ")).map(token => tokenMaps(0).get(token).get).distinct
        val indices1 = tokenizer.tokenize(row(dfColIds(1)).toString.toLowerCase.replace("~", " ")).map(token => tokenMaps(1).get(token).get).distinct
        //        val indices2 = tokenizer.tokenize(row(dfColIds(2)).toString.toLowerCase.replace("~", " ")).map(token => tokenMaps(2).get(token).get).distinct
        //        val indices3 = tokenizer.tokenize(row(dfColIds(3)).toString.toLowerCase.replace("~", " ")).map(token => tokenMaps(3).get(token).get).distinct
        scala.util.Sorting.quickSort(indices0)
        scala.util.Sorting.quickSort(indices1)
        //        val sv0 = Vectors.sparse(tokenMaps(2).size, indices2, Array.fill(indices2.length)(1))
        //        val sv1 = Vectors.sparse(tokenMaps(3).size, indices3, Array.fill(indices3.length)(1))
        //        val label0 = model0.predict(sv0)
        //        val label1 = model1.predict(sv1)
        val builder = StringBuilder.newBuilder
        builder.append(row(labelColId))
        for (index <- staticColIds.indices) {
          if (row(staticColIds(index)) != null && !row(staticColIds(index)).toString.isEmpty) {
            builder.append(' ').append(index).append(':').append(row(staticColIds(index)))
          }
          print("" + index.toString + " ")
        }
        var offset = staticColIds.length
        println(offset)
        //        builder.append(' ').append(offset).append(':').append(label0)
        //        offset += 1
        //        builder.append(' ').append(offset).append(':').append(label1)
        //        offset += 1
        for (subIndex <- indices0)
          builder.append(' ').append(offset + subIndex).append(':').append(1)
        offset += tokenMaps(0).size
        for (subIndex <- indices1)
          builder.append(' ').append(offset + subIndex).append(':').append(1)
        builder.toString()
      }
    ).repartition(1).saveAsTextFile("kddb_output/libsvm")
  }
}
