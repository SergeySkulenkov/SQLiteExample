#include "widget.h"
#include "ui_widget.h"
#include "sqlite/sqlite3.h"
#include <QFile>
#include <QDebug>

static void upper(sqlite3_context *context,  int argc, sqlite3_value **argv){
    if( argc != 1 ) return;
    switch(sqlite3_value_type(argv[0]))
    {
    case SQLITE_NULL:
    {
        sqlite3_result_text( context, "NULL", 4, SQLITE_STATIC );
        break;
    }
    case SQLITE_TEXT:
    {
        QString str(reinterpret_cast<const char*>(sqlite3_value_text(argv[0])));
        str = str.toUpper();
        const char* cstr = static_cast<char*>( str.toUtf8().data());
        sqlite3_result_text(context, cstr, str.toUtf8().size() , SQLITE_TRANSIENT);
        break;
    }
    default:
        sqlite3_result_text( context, "NULL", 4, SQLITE_STATIC );
        break;
    }
}

static int localeCompare( void* /*arg*/, int len1, const void* data1,  int len2, const void* data2 )
{
    qDebug() << "localeCopare";
    QString string1 = QString::fromUtf8((char*)data1,len1).toUpper();
    QString string2 = QString::fromUtf8((char*)data2,len2).toUpper();
    qDebug() << string1 << "=?=" << string2;
    return QString::localeAwareCompare( string1, string2 );
}

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    ui->lineEdit->setFocus();
    ui->label->setWordWrap(true);
    resotreDataBase();
    updateLabel();

}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_pushButton_2_clicked()
{
    std::string name  = ui->lineEdit->text().trimmed().toStdString();
    if(name.empty())
        return;
    sqlite3* db;
    if(sqlite3_open("example.db", &db) == SQLITE_OK){
        ui->plainTextEdit->appendPlainText("Соедениение с базой данных установлено.");
    }else{
        ui->plainTextEdit->appendPlainText("Не удалось установить соединение с базой данных.");
        return;
    }
    std::string sql = "INSERT INTO users (user_name, name) "
                      "VALUES ('user', '"
                      + name + "');";
    ui->plainTextEdit->appendPlainText("Выполняем запрос: \n");
    ui->plainTextEdit->appendPlainText(sql.data());

    char *zErrMsg = 0;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, 0, &zErrMsg);

    if( rc != SQLITE_OK ){
        ui->plainTextEdit->appendPlainText("Ошибка: ");
        ui->plainTextEdit->appendPlainText(zErrMsg);
        sqlite3_free(zErrMsg);
        return;
    } else {
        ui->plainTextEdit->appendPlainText("Данные добавлены.");
    }
    sql = "SELECT MAX(id) FROM users";
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, sql.c_str() ,-1, &stmt, NULL);
    if( rc != SQLITE_OK ){
        ui->plainTextEdit->appendPlainText("Ошибка: ");
        ui->plainTextEdit->appendPlainText(zErrMsg);
        sqlite3_free(zErrMsg);
    } else {
        if( sqlite3_step(stmt) == SQLITE_ROW )
        {
            int id = sqlite3_column_int(stmt,0);
            ui->plainTextEdit->appendPlainText("Последний id = " + QString::number(id));
            updateLabel();
        }
    }
    sqlite3_close(db);

}


void Widget::on_pushButton_clicked()
{
    std::string name  = ui->lineEdit->text().trimmed().toStdString();
    if(name.empty())
        return;
    sqlite3* db;
    if(sqlite3_open("example.db", &db) == SQLITE_OK){
        ui->plainTextEdit->appendPlainText("Соедениение с базой данных установлено.");
    }else{
        ui->plainTextEdit->appendPlainText("Не удалось установить соединение с базой данных.");
        return;
    }

    sqlite3_create_function(db, "upper", 1, SQLITE_UTF8, NULL, &upper, NULL, NULL);
    sqlite3_create_collation(db, "MYCOLLATE", SQLITE_UTF8, nullptr, &localeCompare );

    //std::string sql = "SELECT * FROM users WHERE upper(name) like upper('%"+name+"%')";
    std::string sql = "SELECT * FROM users WHERE name = '"+name+"' COLLATE MYCOLLATE";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str() ,-1, &stmt, NULL);
    if( rc != SQLITE_OK ){
        ui->plainTextEdit->appendPlainText("Ошибка: ");
        ui->plainTextEdit->appendPlainText(sqlite3_errmsg(db));
        return;
    }

    QString text("Получены данные:\n");
    while(sqlite3_step(stmt) == SQLITE_ROW )
    {
        text += reinterpret_cast<const char*> (sqlite3_column_text(stmt,0));
        text += " ";
        text += reinterpret_cast<const char*> (sqlite3_column_text(stmt,1));
        text += " ";
        text += reinterpret_cast<const char*> (sqlite3_column_text(stmt,2));
        text += "\n";
    }
    sqlite3_close(db);

    ui->plainTextEdit->appendPlainText(text);

}


void Widget::on_pushButton_3_clicked()
{
    std::string name = ui->lineEdit->text().trimmed().toStdString();
    if(name.empty())
        return;
    sqlite3* db;
    if(sqlite3_open("example.db", &db) != SQLITE_OK){
        ui->plainTextEdit->appendPlainText("Не удалось установить соединение с базой данных.");
        return;
    }

    char *zErrMsg = 0;

    std::string sql = "DELETE FROM users WHERE name='"+ name +"'";
    int rc = sqlite3_exec(db, sql.c_str() , NULL, 0, &zErrMsg);
    if( rc != SQLITE_OK ){
        ui->plainTextEdit->appendPlainText("Ошибка: ");
        ui->plainTextEdit->appendPlainText(zErrMsg);
        sqlite3_free(zErrMsg);
        return;
    } else {
        ui->plainTextEdit->appendPlainText("Запрос:");
        ui->plainTextEdit->appendPlainText(sql.data());
        ui->plainTextEdit->appendPlainText("Выполнено.");

    }
    sqlite3_close(db);
    updateLabel();
}

void Widget::resotreDataBase()
{

    if(QFile::exists("example.db")){
        return;
    }
    /* Открываем соединение с БД */
    sqlite3* db;
    if(sqlite3_open("example.db", &db) == SQLITE_OK){
        ui->plainTextEdit->appendPlainText("Соедениение с базой данных установлено.");
    }else{
        ui->plainTextEdit->appendPlainText("Не удалось установить соединение с базой данных.");
        return;
    }

    char *zErrMsg = 0;          //Текст сообщения об ошибке

    /* Создаём таблицу */
    std::string sql = "CREATE TABLE users("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "user_name      TEXT NOT NULL,"
                      "name           TEXT NOT NULL);";

    int rc = sqlite3_exec(db, sql.c_str(), nullptr, 0, &zErrMsg);

    if( rc != SQLITE_OK ){
        ui->plainTextEdit->appendPlainText("Ошибка: ");
        ui->plainTextEdit->appendPlainText(zErrMsg);
        sqlite3_free(zErrMsg);
        return;
    } else {
        ui->plainTextEdit->appendPlainText("Таблица users создана.");
    }

    /* Добавляем данные */
    sql = "INSERT INTO users (user_name, name) "
          "VALUES ('user1', 'Павел'); "
          "INSERT INTO users (user_name, name) "
          "VALUES ('user2', 'Василий'); "
          "INSERT INTO users (user_name, name) "
          "VALUES ('user3', 'Олег'); ";

    rc = sqlite3_exec(db, sql.c_str(), nullptr, 0, &zErrMsg);

    if( rc != SQLITE_OK ){
        ui->plainTextEdit->appendPlainText("Ошибка: ");
        ui->plainTextEdit->appendPlainText(zErrMsg);
        sqlite3_free(zErrMsg);
        return;
    } else {
        ui->plainTextEdit->appendPlainText("Данные добавлены.");
    }

    /* Закрываем соединение с БД */
    sqlite3_close(db);

}

void Widget::updateLabel()
{
    ui->label->clear();

    sqlite3* db;
    if(sqlite3_open("example.db", &db) != SQLITE_OK){
        ui->plainTextEdit->appendPlainText("Не удалось установить соединение с базой данных.");
        return;
    }


    std::string sql = "SELECT * FROM users";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql.c_str() ,-1, &stmt, NULL);

    if( rc != SQLITE_OK ){
        ui->plainTextEdit->appendPlainText("Ошибка: ");
        ui->plainTextEdit->appendPlainText(sqlite3_errmsg(db));
    } else {
        QString text("Данные:\nid\tuser\tname\n");
        while( sqlite3_step(stmt) == SQLITE_ROW )
        {
            text += reinterpret_cast<const char*> (sqlite3_column_text(stmt,0));
            text += "\t";
            text += reinterpret_cast<const char*> (sqlite3_column_text(stmt,1));
            text += "\t";
            text += reinterpret_cast<const char*> (sqlite3_column_text(stmt,2));
            text += "\n";
        }

        ui->label->setText(text);
    }

    /* Закрываем соединение с БД */
    sqlite3_close(db);
}

