#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>

#include "src/ArchiverCompatibility.hpp"
#include "src/Tables/Tables.hpp"

namespace {

std::uint64_t HashBytes(const unsigned char* pBytes, std::size_t pLength) {
  std::uint64_t aDigest = 1469598103934665603ULL;
  for (std::size_t aIndex = 0; aIndex < pLength; ++aIndex) {
    aDigest = (aDigest * 1099511628211ULL) ^ static_cast<std::uint64_t>(pBytes[aIndex]);
  }
  return aDigest;
}

std::uint64_t HashValue(std::uint64_t pDigest, std::uint64_t pValue) {
  for (int aShift = 0; aShift < 64; aShift += 8) {
    pDigest = (pDigest * 1099511628211ULL) ^ ((pValue >> aShift) & 0xFFU);
  }
  return pDigest;
}

QString FormatFraction(double pFraction) {
  std::ostringstream aStream;
  aStream << std::fixed << std::setprecision(1) << (pFraction * 100.0) << "%";
  return QString::fromStdString(aStream.str());
}

class LaunchWorker final : public QObject, public peanutbutter::archiver::Logger {
  Q_OBJECT

 public slots:
  void Run(const QString& pPassword, int pVersion, int pGameStyle, int pMazeStyle, bool pIsFastMode) {
    const QByteArray aPasswordBytes = pPassword.toUtf8();
    const auto aStartedAt = std::chrono::steady_clock::now();

    peanutbutter::archiver::LaunchRequest aRequest;
    aRequest.mPassword = reinterpret_cast<unsigned char*>(const_cast<char*>(aPasswordBytes.data()));
    aRequest.mPasswordLength = aPasswordBytes.size();
    aRequest.mExpanderVersion = static_cast<std::uint8_t>(pVersion);
    aRequest.mGameStyle = static_cast<peanutbutter::archiver::GameStyle>(pGameStyle);
    aRequest.mMazeStyle = static_cast<peanutbutter::archiver::MazeStyle>(pMazeStyle);
    aRequest.mIsFastMode = pIsFastMode;
    aRequest.mLogger = this;
    aRequest.mShouldCancel = nullptr;
    aRequest.mCancelUserData = nullptr;

    const bool aSuccess = peanutbutter::archiver::Launch(aRequest);

    const auto aEndedAt = std::chrono::steady_clock::now();
    const qint64 aElapsedMilliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(aEndedAt - aStartedAt).count();

    std::uint64_t aDigest = 1469598103934665603ULL;
    for (const auto& aTable : peanutbutter::tables::Tables::All()) {
      aDigest = HashValue(aDigest, HashBytes(aTable.mData, aTable.mSize));
    }

    emit Finished(aSuccess, aElapsedMilliseconds, QString::number(aDigest));
  }

 signals:
  void Status(const QString& pMessage);
  void Error(const QString& pMessage);
  void Progress(double pFraction, const QString& pDetail);
  void Finished(bool pSuccess, qint64 pElapsedMilliseconds, const QString& pDigest);

 private:
  void LogStatus(const std::string& pMessage) override {
    emit Status(QString::fromStdString(pMessage));
  }

  void LogError(const std::string& pMessage) override {
    emit Error(QString::fromStdString(pMessage));
  }

  void LogProgress(const peanutbutter::archiver::ProgressInfo& pInfo) override {
    emit Progress(pInfo.mOverallFraction, QString::fromStdString(pInfo.mDetail));
  }
};

class MainWindow final : public QWidget {
  Q_OBJECT

 public:
  MainWindow()
      : mPasswordEdit(new QLineEdit(QStringLiteral("hotdog"), this)),
        mVersionSpin(new QSpinBox(this)),
        mGameStyleCombo(new QComboBox(this)),
        mMazeStyleCombo(new QComboBox(this)),
        mFastCheck(new QCheckBox(QStringLiteral("Fast Mode"), this)),
        mRunButton(new QPushButton(QStringLiteral("Run Expansion"), this)),
        mSpinner(new QProgressBar(this)),
        mPercentLabel(new QLabel(QStringLiteral("0.0%"), this)),
        mLogBox(new QPlainTextEdit(this)),
        mWorker(new LaunchWorker()),
        mWorkerThread(new QThread(this)) {
    setWindowTitle(QStringLiteral("Bread Tables Shell"));
    resize(860, 640);

    mVersionSpin->setRange(0, 255);
    mVersionSpin->setValue(peanutbutter::archiver::ExpanderLibraryVersion());

    mGameStyleCombo->addItem(QStringLiteral("None"), static_cast<int>(peanutbutter::archiver::GameStyle::kNone));
    mGameStyleCombo->addItem(QStringLiteral("Sparse"), static_cast<int>(peanutbutter::archiver::GameStyle::kSparse));
    mGameStyleCombo->addItem(QStringLiteral("Full"), static_cast<int>(peanutbutter::archiver::GameStyle::kFull));

    mMazeStyleCombo->addItem(QStringLiteral("None"), static_cast<int>(peanutbutter::archiver::MazeStyle::kNone));
    mMazeStyleCombo->addItem(QStringLiteral("Sparse"), static_cast<int>(peanutbutter::archiver::MazeStyle::kSparse));
    mMazeStyleCombo->addItem(QStringLiteral("Full"), static_cast<int>(peanutbutter::archiver::MazeStyle::kFull));

    mSpinner->setRange(0, 1);
    mSpinner->setValue(0);
    mLogBox->setReadOnly(true);

    auto* aFormLayout = new QFormLayout();
    aFormLayout->addRow(QStringLiteral("Password"), mPasswordEdit);
    aFormLayout->addRow(QStringLiteral("Expander Version"), mVersionSpin);
    aFormLayout->addRow(QStringLiteral("Game Style"), mGameStyleCombo);
    aFormLayout->addRow(QStringLiteral("Maze Style"), mMazeStyleCombo);
    aFormLayout->addRow(QString(), mFastCheck);

    auto* aProgressRow = new QHBoxLayout();
    aProgressRow->addWidget(mSpinner, 1);
    aProgressRow->addWidget(mPercentLabel);

    auto* aRootLayout = new QVBoxLayout(this);
    aRootLayout->addLayout(aFormLayout);
    aRootLayout->addWidget(mRunButton);
    aRootLayout->addLayout(aProgressRow);
    aRootLayout->addWidget(mLogBox, 1);

    mWorker->moveToThread(mWorkerThread);
    connect(mWorkerThread, &QThread::finished, mWorker, &QObject::deleteLater);
    connect(mRunButton, &QPushButton::clicked, this, &MainWindow::OnRunClicked);
    connect(mWorker, &LaunchWorker::Status, this, &MainWindow::AppendStatus);
    connect(mWorker, &LaunchWorker::Error, this, &MainWindow::AppendError);
    connect(mWorker, &LaunchWorker::Progress, this, &MainWindow::UpdateProgress);
    connect(mWorker, &LaunchWorker::Finished, this, &MainWindow::OnFinished);
    mWorkerThread->start();
  }

  ~MainWindow() override {
    mWorkerThread->quit();
    mWorkerThread->wait();
  }

 private slots:
  void OnRunClicked() {
    mLogBox->clear();
    mRunButton->setEnabled(false);
    mSpinner->setRange(0, 0);
    mPercentLabel->setText(QStringLiteral("0.0%"));

    QMetaObject::invokeMethod(mWorker,
                              "Run",
                              Qt::QueuedConnection,
                              Q_ARG(QString, mPasswordEdit->text()),
                              Q_ARG(int, mVersionSpin->value()),
                              Q_ARG(int, mGameStyleCombo->currentData().toInt()),
                              Q_ARG(int, mMazeStyleCombo->currentData().toInt()),
                              Q_ARG(bool, mFastCheck->isChecked()));
  }

  void AppendStatus(const QString& pMessage) {
    mLogBox->appendPlainText(pMessage);
  }

  void AppendError(const QString& pMessage) {
    mLogBox->appendPlainText(pMessage);
  }

  void UpdateProgress(double pFraction, const QString& pDetail) {
    mPercentLabel->setText(FormatFraction(pFraction));
    if (!pDetail.isEmpty()) {
      mLogBox->appendPlainText(QStringLiteral("[Progress] %1 %2").arg(FormatFraction(pFraction), pDetail));
    }
  }

  void OnFinished(bool pSuccess, qint64 pElapsedMilliseconds, const QString& pDigest) {
    mSpinner->setRange(0, 1000);
    mSpinner->setValue(1000);
    mPercentLabel->setText(QStringLiteral("100.0%"));
    mRunButton->setEnabled(true);
    mLogBox->appendPlainText(QStringLiteral("[Result] success=%1 elapsed_ms=%2 digest=%3")
                                 .arg(pSuccess ? QStringLiteral("true") : QStringLiteral("false"))
                                 .arg(pElapsedMilliseconds)
                                 .arg(pDigest));
  }

 private:
  QLineEdit* mPasswordEdit;
  QSpinBox* mVersionSpin;
  QComboBox* mGameStyleCombo;
  QComboBox* mMazeStyleCombo;
  QCheckBox* mFastCheck;
  QPushButton* mRunButton;
  QProgressBar* mSpinner;
  QLabel* mPercentLabel;
  QPlainTextEdit* mLogBox;
  LaunchWorker* mWorker;
  QThread* mWorkerThread;
};

}  // namespace

int main(int pArgc, char** pArgv) {
  QApplication aApp(pArgc, pArgv);
  MainWindow aWindow;
  aWindow.show();
  return aApp.exec();
}

#include "main_qt.moc"
