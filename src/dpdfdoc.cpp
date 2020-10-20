#include "dpdfdoc.h"
#include "dpdfpage.h"

#include "public/fpdfview.h"
#include "public/fpdf_doc.h"
#include "public/fpdf_save.h"

#include "core/fpdfdoc/cpdf_bookmark.h"
#include "core/fpdfdoc/cpdf_bookmarktree.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfdoc/cpdf_pagelabel.h"

#include <QFile>
#include <iostream>
#include <string.h>
#include <QTemporaryFile>

DPdfDoc::Status parseError(int error)
{
    DPdfDoc::Status err_code = DPdfDoc::SUCCESS;
    // Translate FPDFAPI error code to FPDFVIEW error code
    switch (error) {
    case FPDF_ERR_SUCCESS:
        err_code = DPdfDoc::SUCCESS;
        break;
    case FPDF_ERR_FILE:
        err_code = DPdfDoc::FILE_ERROR;
        break;
    case FPDF_ERR_FORMAT:
        err_code = DPdfDoc::FORMAT_ERROR;
        break;
    case FPDF_ERR_PASSWORD:
        err_code = DPdfDoc::PASSWORD_ERROR;
        break;
    case FPDF_ERR_SECURITY:
        err_code = DPdfDoc::HANDLER_ERROR;
        break;
    }
    return err_code;
}

class DPdfDocPrivate
{
    friend class DPdfDoc;
public:
    DPdfDocPrivate();

    ~DPdfDocPrivate();

public:
    DPdfDoc::Status loadFile(const QString &filename, const QString &password);

private:
    DPdfDocHandler *m_docHandler;

    QVector<DPdfPage *> m_pages;

    QString m_filename;

    int m_pageCount = 0;

    DPdfDoc::Status m_status;
};

DPdfDocPrivate::DPdfDocPrivate()
{
    m_docHandler = nullptr;
    m_pageCount = 0;
    m_status = DPdfDoc::NOT_LOADED;
}

DPdfDocPrivate::~DPdfDocPrivate()
{
    m_pages.clear();

    if (nullptr != m_docHandler)
        FPDF_CloseDocument((FPDF_DOCUMENT)m_docHandler);
}

DPdfDoc::Status DPdfDocPrivate::loadFile(const QString &filename, const QString &password)
{
    m_filename = filename;

    m_pages.clear();

    if (!QFile::exists(filename)) {
        m_status = DPdfDoc::FILE_NOT_FOUND_ERROR;
        return m_status;
    }

    void *ptr = FPDF_LoadDocument(m_filename.toUtf8().constData(),
                                  password.toUtf8().constData());

    m_docHandler = static_cast<DPdfDocHandler *>(ptr);

    m_status = m_docHandler ? DPdfDoc::SUCCESS : parseError(FPDF_GetLastError());

    if (m_docHandler) {
        m_pageCount = FPDF_GetPageCount((FPDF_DOCUMENT)m_docHandler);
        m_pages.fill(nullptr, m_pageCount);
    }

    return m_status;
}


DPdfDoc::DPdfDoc(QString filename, QString password)
    : d_ptr(new DPdfDocPrivate())
{
    d_func()->loadFile(filename, password);
}

DPdfDoc::~DPdfDoc()
{

}

bool DPdfDoc::isValid() const
{
    return d_func()->m_docHandler != nullptr;
}

bool DPdfDoc::isEncrypted() const
{
    if (!isValid())
        return false;

    return FPDF_GetDocPermissions((FPDF_DOCUMENT)d_func()->m_docHandler) != 0xFFFFFFFF;
}

DPdfDoc::Status DPdfDoc::tryLoadFile(const QString &filename, const QString &password)
{
    Status status = NOT_LOADED;
    if (!QFile::exists(filename)) {
        status = FILE_NOT_FOUND_ERROR;
        return status;
    }

    void *ptr = FPDF_LoadDocument(filename.toUtf8().constData(),
                                  password.toUtf8().constData());

    DPdfDocHandler *docHandler = static_cast<DPdfDocHandler *>(ptr);
    status = docHandler ? SUCCESS : parseError(FPDF_GetLastError());

    if (docHandler) {
        FPDF_CloseDocument((FPDF_DOCUMENT)docHandler);
    }

    return status;
}

static QFile saveWriter;

int writeFile(struct FPDF_FILEWRITE_* pThis, const void *pData, unsigned long size)
{
    Q_UNUSED(pThis)
    return 0 != saveWriter.write(static_cast<char *>(const_cast<void *>(pData)), static_cast<qint64>(size));
}

bool DPdfDoc::save()
{
    FPDF_FILEWRITE write;

    write.WriteBlock = writeFile;

    saveWriter.setFileName(d_func()->m_filename);

    if (!saveWriter.open(QIODevice::ReadWrite))
        return false;

    bool result = FPDF_SaveAsCopy(reinterpret_cast<FPDF_DOCUMENT>(d_func()->m_docHandler), &write, FPDF_REMOVE_SECURITY);

    saveWriter.close();

    return result;
}

bool DPdfDoc::saveAs(const QString &filePath)
{
    FPDF_FILEWRITE write;

    write.WriteBlock = writeFile;

    saveWriter.setFileName(filePath);

    if (!saveWriter.open(QIODevice::ReadWrite))
        return false;

    bool result = FPDF_SaveAsCopy(reinterpret_cast<FPDF_DOCUMENT>(d_func()->m_docHandler), &write, FPDF_REMOVE_SECURITY);

    saveWriter.close();

    return result;
}

QString DPdfDoc::filename() const
{
    return d_func()->m_filename;
}

int DPdfDoc::pageCount() const
{
    return d_func()->m_pageCount;
}

DPdfDoc::Status DPdfDoc::status() const
{
    return d_func()->m_status;
}

DPdfPage *DPdfDoc::page(int i)
{
    if (i < 0 || i >= d_func()->m_pageCount)
        return nullptr;

    if (!d_func()->m_pages[i]) {
        d_func()->m_pages[i] = new DPdfPage(d_func()->m_docHandler, i);
    }

    return d_func()->m_pages[i];
}

QSizeF DPdfDoc::pageSizeF(int index) const
{
    double width = 0;
    double height = 0;

    FPDF_GetPageSizeByIndex((FPDF_DOCUMENT)d_func()->m_docHandler, index, &width, &height);
    return QSizeF(width, height);
}

void collectBookmarks(DPdfDoc::Outline &outline, const CPDF_BookmarkTree &tree, CPDF_Bookmark This)
{
    DPdfDoc::Section section;

    const WideString &title = This.GetTitle();
    section.title = QString::fromWCharArray(title.c_str(), title.GetLength());

    bool hasx = false, hasy = false, haszoom = false;
    float x = 0.0, y = 0.0, z = 0.0;

    const CPDF_Dest &dest = This.GetDest(tree.GetDocument()).GetArray() ? This.GetDest(tree.GetDocument()) : This.GetAction().GetDest(tree.GetDocument());
    section.nIndex = dest.GetDestPageIndex(tree.GetDocument());
    dest.GetXYZ(&hasx, &hasy, &haszoom, &x, &y, &z);
    section.offsetPointF = QPointF(static_cast<qreal>(x), static_cast<qreal>(y));

    const CPDF_Bookmark &Child = tree.GetFirstChild(&This);
    if (Child.GetDict() != nullptr) {
        collectBookmarks(section.children, tree, Child);
    }
    outline << section;

    const CPDF_Bookmark &SibChild = tree.GetNextSibling(&This);
    if (SibChild.GetDict() != nullptr) {
        collectBookmarks(outline, tree, SibChild);
    }
}

DPdfDoc::Outline DPdfDoc::outline()
{
    Outline outline;
    CPDF_BookmarkTree tree(reinterpret_cast<CPDF_Document *>(d_func()->m_docHandler));
    CPDF_Bookmark cBookmark;
    const CPDF_Bookmark &firstRootChild = tree.GetFirstChild(&cBookmark);
    if (firstRootChild.GetDict() != nullptr)
        collectBookmarks(outline, tree, firstRootChild);

    return outline;
}

DPdfDoc::Properies DPdfDoc::proeries()
{
    Properies properies;
    int fileversion = 1;
    properies.insert("Version", "1");
    if (FPDF_GetFileVersion((FPDF_DOCUMENT)d_func()->m_docHandler, &fileversion)) {
        properies.insert("Version", QString("%1.%2").arg(fileversion / 10).arg(fileversion % 10));
    }
    properies.insert("Encrypted", isEncrypted());
    properies.insert("Linearized", FPDF_GetFileLinearized((FPDF_DOCUMENT)d_func()->m_docHandler));

    properies.insert("KeyWords", QString());
    properies.insert("Title", QString());
    properies.insert("Creator", QString());
    properies.insert("Producer", QString());
    CPDF_Document *pDoc = reinterpret_cast<CPDF_Document *>(d_func()->m_docHandler);
    const CPDF_Dictionary *pInfo = pDoc->GetInfo();
    if (pInfo) {
        const WideString &KeyWords = pInfo->GetUnicodeTextFor("Keywords");
        properies.insert("KeyWords", QString::fromWCharArray(KeyWords.c_str(), KeyWords.GetLength()));

        const WideString &Title = pInfo->GetUnicodeTextFor("Title");
        properies.insert("Title", QString::fromWCharArray(Title.c_str(), Title.GetLength()));

        const WideString &Creator = pInfo->GetUnicodeTextFor("Creator");
        properies.insert("Creator", QString::fromWCharArray(Creator.c_str(), Creator.GetLength()));

        const WideString &Producer = pInfo->GetUnicodeTextFor("Producer");
        properies.insert("Producer", QString::fromWCharArray(Producer.c_str(), Producer.GetLength()));
    }

    return properies;
}

QString DPdfDoc::label(int index) const
{
    CPDF_PageLabel label(reinterpret_cast<CPDF_Document *>(d_func()->m_docHandler));
    const Optional<WideString> &str = label.GetLabel(index);
    if (str.has_value())
        return QString::fromWCharArray(str.value().c_str(), str.value().GetLength());
    return QString();
}
