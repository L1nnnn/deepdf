#include "dpdfiumpage.h"
#include "public/fpdfview.h"
#include "public/fpdf_text.h"
#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdftext/cpdf_textpage.h"

PageHolder::PageHolder(QWeakPointer<CPDF_Document> doc, CPDF_Page *page)
    : m_doc(doc)
    , m_page(page)
    , m_textPage(static_cast<CPDF_TextPage*>(FPDFText_LoadPage(page)))
{
}

PageHolder::~PageHolder()
{
    if (m_page)
        FPDF_ClosePage(m_page);
    if (m_textPage)
        FPDFText_ClosePage(m_textPage);
}

DPdfiumPage::DPdfiumPage(QSharedPointer<PageHolder> page, int index)
    : m_pageHolder(page)
    , m_index(index)
{
}

DPdfiumPage::DPdfiumPage(const DPdfiumPage &other)
    : m_pageHolder(other.m_pageHolder)
    , m_index(other.m_index)
{
}

DPdfiumPage &DPdfiumPage::operator=(const DPdfiumPage &other)
{
    m_pageHolder = other.m_pageHolder;
    m_index = other.m_index;
    return *this;
}

DPdfiumPage::~DPdfiumPage()
{

}

qreal DPdfiumPage::width() const
{
    if (!m_pageHolder)
        return -1;
    return m_pageHolder.data()->m_page->GetPageWidth();
}

qreal DPdfiumPage::height() const
{
    if (!m_pageHolder)
        return -1;
    return m_pageHolder.data()->m_page->GetPageHeight();
}


bool DPdfiumPage::isValid() const
{
    return !m_pageHolder.isNull() && !m_pageHolder->m_doc.isNull();
}

int DPdfiumPage::pageIndex() const
{
    return m_index;
}

QImage DPdfiumPage::image(qreal scale)
{
    if (!isValid())
        return QImage();

    //We need to hold the document while generating the image
    QSharedPointer<CPDF_Document> d = m_pageHolder->m_doc.toStrongRef();

    if (!d)
        return QImage();

    QImage image(width()*scale, height()*scale, QImage::Format_RGBA8888);

    if(image.isNull())
        return QImage();

    image.fill(0xFFFFFFFF);

    FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(image.width(), image.height(),
                                             FPDFBitmap_BGRA,
                                             image.scanLine(0), image.bytesPerLine());
    if(bitmap == NULL) {
        return QImage();
    }

    FPDF_RenderPageBitmap(bitmap, m_pageHolder.data()->m_page,
                          0, 0, image.width(), image.height(),
                          0, 0); // no rotation, no flags
    FPDFBitmap_Destroy(bitmap);
    bitmap = NULL;

    for(int i = 0; i < image.height(); i++) {
        uchar *pixels = image.scanLine(i);
        for(int j = 0; j < image.width(); j++) {
            qSwap(pixels[0], pixels[2]);
            pixels += 4;
        }
    }

    return image;
}

int DPdfiumPage::countChars() const
{
    return m_pageHolder->m_textPage->CountChars();
}

QVector<QRectF> DPdfiumPage::getTextRects(int start, int count) const
{
    QVector<QRectF> result;
    std::vector<CFX_FloatRect> pdfiumRects = m_pageHolder->m_textPage->GetRectArray(start, count);
    result.reserve(pdfiumRects.size());
    for (CFX_FloatRect &rect: pdfiumRects) {
        // QRectF coordinates have their origin point top left instead of bottom left for CFX_FloatRect
        result.push_back({rect.left, height() - rect.top, rect.right - rect.left, rect.top - rect.bottom});
    }
    return result;
}

QString DPdfiumPage::text(const QRectF& rect) const
{
    // QRectF coordinates have their origin point top left instead of bottom left for CFX_FloatRect, 
    // so here we reverse the symetry done in getTextRects.
    qreal newBottom = height() - rect.bottom();
    qreal newTop = height() - rect.top();
    CFX_FloatRect fxRect(rect.left(), std::min(newBottom, newTop), rect.right(), std::max(newBottom, newTop));
    auto text = m_pageHolder->m_textPage->GetTextByRect(fxRect);
    return QString::fromWCharArray(text.c_str(), text.GetLength());
}

QString DPdfiumPage::text() const
{
    return text(0, countChars());
}

QString DPdfiumPage::text(int start, int charCount) const
{
    auto text = m_pageHolder->m_textPage->GetPageText(start, charCount);
    return QString::fromWCharArray(text.c_str(), charCount);
}

