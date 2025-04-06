package com.craftinginterpreters.tool;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.List;

public class GenerateAst {
    public static void main(String[] args) throws IOException {
        if (args.length != 1) {
            System.err.println("Usage: ast <output directory>");
            System.exit(1);
        }

        String outputDir = args[0];

        defineAst(outputDir, "Expr", Arrays.asList(
            "Binary   : Expr left, Token operator, Expr right",
            "Grouping : Expr expression",
            "Literal  : Object value",
            "Unary    : Token operator, Expr right"
        ));

        defineAst(outputDir, "Stmt", Arrays.asList(
            "Expression : Expr expression",
            "Print      : Expr expression"
        ));
    }

    // Expr / Stmt の継承（ポリモーフィズム）により，型判定のための分岐をなくす．
    // Visitor パターンにより，呼び出し元判定のための分岐をなくす．
    // これにより，同じデータ構造に対して，異なるコンテキストで異なる処理を行いたい場合に，データ構造と処理を分離することができる．
    // つまり，OOP言語でありながら関数型言語的スタイルに接近することができる．
    private static void defineAst(
        String outputDir,
        String baseName,
        List<String> types
    ) throws IOException {
        String path = outputDir + "/" + baseName + ".java";
        PrintWriter writer = new PrintWriter(path, "UTF-8");
    
        writer.println("package com.craftinginterpreters.lox;");
        writer.println();
        writer.println("import java.util.List;");
        writer.println();
        writer.println("abstract class " + baseName + " {");

        // 規定クラスの accept メソッド
        writer.println("  abstract <R> R accept(Visitor<R> visitor);");
        writer.println();

        defineVisitor(writer, baseName, types);
    
        // Ast クラス群
        for (String type : types) {
            String className = type.split(":")[0].trim();
            String fields = type.split(":")[1].trim();
            defineType(writer, baseName, className, fields);
        }
    
        writer.println("}");
        writer.close();
    }
    
    private static void defineType(
        PrintWriter writer,
        String baseName,
        String className,
        String fieldList
    ) {
        writer.println("  static class " + className + " extends " + baseName + " {");
    
        // フィールド
        String[] fields = fieldList.split(", ");
        for (String field : fields) {
            writer.println("    final " + field + ";");
        }
        writer.println();
    
        // コンストラクタ
        writer.println("    " + className + "(" + fieldList + ") {");
        for (String field : fields) {
            String name = field.split(" ")[1];
            writer.println("      this." + name + " = " + name + ";");
        }
        writer.println("    }");

        // Visitor パターン
        writer.println();
        writer.println("    @Override");
        writer.println("    <R> R accept(Visitor<R> visitor) {");
        writer.println("      return visitor.visit" + className + baseName + "(this);");
        writer.println("    }");
    
        writer.println("  }");
        writer.println();
    }

    private static void defineVisitor(
        PrintWriter writer,
        String baseName,
        List<String> types
    ) {
        writer.println("  interface Visitor<R> {");
    
        for (String type : types) {
            String typeName = type.split(":")[0].trim();
            writer.println("    R visit" + typeName + baseName + "(" + typeName + " " + baseName.toLowerCase() + ");");
        }
    
        writer.println("  }");
        writer.println();
    }
}

