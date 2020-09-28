#ifndef DPDFPAGE_H
#define DPDFPAGE_H

#include <QObject>
#include <QImage>
#include <QScopedPointer>

#include "dpdfglobal.h"

class DPdfium;
class DPdfAnnot;
class DPdfPagePrivate;
class DPdfDocHandler;
class DEEPIN_PDFIUM_EXPORT DPdfPage : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(DPdfPage)
    friend class DPdfDoc;

public:
    struct Link {
        int nIndex = -1;
        QString urlpath;

        bool isValid()
        {
            return nIndex >= 0;
        }
    };

public:
    ~DPdfPage();

    /**
     * @brief 图片宽
     * @return
     */
    qreal width() const;

    /**
     * @brief 图片高
     * @return
     */
    qreal height() const;

    /**
     * @brief 当页索引
     * @return
     */
    int pageIndex() const;

    /**
     * @brief 获取范围内图片
     * @param scale
     * @return
     */
    QImage image(qreal xscale = 1, qreal yscale = 1, qreal x = 0, qreal y = 0, qreal width = 0, qreal height = 0);

    /**
     * @brief 字符数
     * @return
     */
    int countChars() const;

    /**
     * @brief 根据索引获取文本范围
     * @param start
     * @param charCount
     * @return
     */
    QVector<QRectF> getTextRects(int start = 0, int charCount = -1) const;

    /**
     * @brief 根据范围获取文本
     * @param rect
     * @return
     */
    QString text(const QRectF &rect) const;

    /**
     * @brief 获取总文本
     * @return
     */
    QString text() const;

    /**
     * @brief 根据索引获取文本
     * @param start
     * @param charCount
     * @return
     */
    QString text(int start, int charCount) const;

    /**
     * @brief 下标真实页码
     * @return
     */
    QString label() const;

    /**
     * @brief 获取当前支持操作的所有注释
     * @return 注释列表，只会列出已支持的注释
     */
    QList<DPdfAnnot *> annots();

    /**
     * @brief 获取位置所在Link
     * @return
     */
    Link getLinkAtPoint(qreal x, qreal y);

    /**
     * @brief 添加注释
     * @param color 传空则不添加颜色
     * @param boudary 传空则不添加位置 基于坐标系(0~1)
     * @return
     */
    bool createAnnot(int annotType, QColor color = QColor(), QRectF boudary = QRectF());

    /**
     * @brief 更新注释
     * @param annot 给更新的注释指针，执行成功传入的指针会被删除
     * @param color 传空则不更新颜色
     * @param boudary 传空则不更新位置 基于坐标系(0~1)
     * @return
     */
    bool updateAnnot(DPdfAnnot *dAnnot, QColor color = QColor(), QRectF boudary = QRectF());

    /**
     * @brief 删除注释
     * @param annot 即将删除的注释指针，执行成功传入的指针会被删除
     * @return
     */
    bool removeAnnot(DPdfAnnot *dAnnot);

signals:
    /**
     * @brief 添加注释时触发 ，在需要的时候可以重新获取annotations()
     * @param annot 新增加的annot
     */
    void annotAdded(DPdfAnnot *dAnnot);

    /**
     * @brief 注释被更新时触发 ，在需要的时候可以重新获取annotations()
     * @param annot 被更新的annot
     */
    void annotUpdated(DPdfAnnot *dAnnot);

    /**
     * @brief 注释被删除时触发 ，在需要的时候可以重新获取annotations()
     * @param annot 被移除的annot 注意这个已经是个将要被析构后的地址 只用于做匹配移除
     */
    void annotRemoved(DPdfAnnot *dAnnot);

private:
    DPdfPage(DPdfDocHandler *handler, int pageIndex);

    QScopedPointer<DPdfPagePrivate> d_ptr;
};

#endif // DPDFPAGE_H
